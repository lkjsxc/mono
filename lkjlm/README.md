# Simple Character-Level Language Model in C

This project implements a basic character-level language model using C. It employs an evolutionary strategy for training its parameters. The model processes text character by character, attempting to predict the next character in a sequence. It features a simple two-layer neural network-like architecture with a "memory" component and supports multi-threaded training.

## Features

*   **Character-level processing:** Operates directly on raw byte values as characters.
*   **Evolutionary Training:** Uses a genetic algorithm-like approach where parameters are mutated, and better-performing sets are kept.
*   **Multi-threaded Training:** Leverages `pthreads` to parallelize the evaluation of different parameter sets, speeding up training.
*   **Configurable Architecture:** Layer sizes, mutation rates, and other hyperparameters are defined as macros.
*   **"Think" Mechanism:** The model can output a special `THINK_TOKEN` (`\t`) to perform internal calculations without producing an output character, potentially allowing for more complex sequence generation.
*   **Simple Feed-Forward Layers:** Uses a basic calculation for layer propagation, involving weighted sums and a scaling shift.
*   **File I/O:** Reads training data, input prompts, and writes generated output to files.

## How it Works

### 1. Model Architecture

*   **Layers:** The model uses two primary "layers" (`layer1_u8`, `layer2_u8`). Each layer is a byte array of size `LAYER_BYTESIZE`.
    *   `LAYERIO_BYTESIZE` (256 bytes): The first part of the layer is used for input/output. When setting a character, its ASCII value is used as an index, and that byte is set to `UINT8_MAX` (one-hot like encoding). When getting a character, the index with the highest byte value in this I/O section is chosen.
    *   `MEMORY_BYTESIZE`: The second part of the layer acts as a recurrent memory or hidden state.
*   **Parameters (`param_i8`):**
    *   A large array of `int8_t` values (`PARAM_BYTESIZE`).
    *   `PARAM_BYTESIZE` is calculated as `LAYER_BYTESIZE * (LAYER_BYTESIZE + 1)`. This suggests a fully connected-like transformation where each neuron in the output layer receives input from all neurons in the input layer, plus a bias term for each output neuron.
*   **Layer Calculation (`layer_cal`):**
    *   Transforms `layer1_u8` into `layer2_u8` using the `param_i8`.
    *   For each byte in `layer2_u8`, it computes a sum: `bias + sum(layer1_u8[j] * param[k])`.
    *   The sum is scaled down (`>> LAYER_CAL_SCALE_SHIFT`) and clamped to `[0, UINT8_MAX]`.
    *   After calculation, `layer1_u8` and `layer2_u8` pointers are swapped, making the previous output the new input for the next step.

### 2. Training (`thread_func`, `evaluate`)

*   **Evolutionary Strategy:**
    1.  A global `best_param` set is maintained.
    2.  Multiple threads are created. Each thread:
        a.  Copies `best_param` to its local `param_bytes`.
        b.  Applies mutations (`param_update`): randomly flips bits in its local parameters based on `MUTATION_RATE`.
        c.  Evaluates its mutated parameters (`evaluate`) on the training dataset.
*   **Evaluation (`evaluate`):**
    1.  The training data (`train.txt`) is parsed into prompt/response line pairs.
    2.  For each pair:
        a.  The model is fed the prompt characters one by one.
        b.  Then, it attempts to predict each character of the response.
        c.  A `score` is calculated (`layer_score`):
            *   Rewards high activation for the `correct_char`.
            *   Rewards (lesser) high activation for the `THINK_TOKEN`.
            *   Gives a significant bonus for an `exact_match`.
            *   Penalizes overall high activation in the I/O part of the layer (encouraging sparse output).
            *   Includes a small reward based on popcount in the memory part, trying to encourage a certain level of memory activation.
        d.  If the model predicts `THINK_TOKEN`, it gets a small reward, and the current response character is not consumed (the model "thinks" for a step). This is limited by `THINK_LIMIT`.
        e.  If the model makes too many consecutive incorrect predictions (`PRUNING_LIMIT`), evaluation for that pair is stopped.
    3.  If a thread's mutated parameters achieve a score higher than the global `best_score`, it updates `global_data.best_param` (protected by a mutex).
*   This process repeats for `TRAIN_COUNT` cycles, distributed among `THREAD_COUNT` threads.

### 3. Generation (`generate_output`)

1.  The `best_param` found during training is used.
2.  The model is primed by feeding it the contents of `input.txt`.
3.  It then generates `OUTPUT_COUNT` characters:
    a.  Get the predicted character using `layer_getchar`.
    b.  If it's not `THINK_TOKEN`, append it to the output buffer.
    c.  Feed the predicted character (or the actual character if not thinking) back into the model using `layer_setchar`.
    d.  Perform `layer_cal`.
4.  The generated sequence is written to `output.txt`.

## Configuration (Macros)

Key configuration parameters are defined as macros at the top of the C file:

*   `LAYERIO_BYTESIZE`: Size of the I/O part of a layer (default: 256, for 8-bit characters).
*   `MEMORY_BYTESIZE`: Size of the memory/internal state part of a layer.
*   `MUTATION_RATE`: Probability of a bit in the parameters being flipped during mutation.
*   `PRUNING_LIMIT`: Max consecutive wrong predictions before stopping evaluation for a training sample.
*   `THINK_LIMIT`: Max consecutive "think" steps allowed during evaluation/generation.
*   `THINK_REWARD`: Score reward for a "think" step.
*   `SCORE_REWARD_CORRECT_ACTIVATION`, `SCORE_REWARD_THINK_ACTIVATION`, `SCORE_BONUS_EXACT_MATCH`: Scoring constants.
*   `SCORE_PENALTY_PER_ACTIVATION`: Penalty for each active output neuron.
*   `THREAD_COUNT`: Number of parallel threads for training.
*   `TRAIN_COUNT`: Total number of training iterations (distributed among threads).
*   `OUTPUT_COUNT`: Number of characters to generate after training.
*   `RANDOM_SEED`: Seed for the random number generator. `0` uses `time(NULL)`.
*   `TEXT_BYTESIZE`: Max buffer size for train/input/output text data.
*   `TEXT_SETSIZE`: Max number of prompt/response line pairs in training data.
*   `TRAIN_PATH`, `INPUT_PATH`, `OUTPUT_PATH`: File paths for data.
*   `PARAM_PATH`: Defined but not currently used to save/load best parameters to disk.
*   `THINK_TOKEN`: Character used for the "think" action (default: `\t`).
*   `LAYER_CAL_SCALE_SHIFT`: Right bit-shift amount for scaling down sums in layer calculation.

## File Structure

*   `main.c` (or your chosen filename): The source code.
*   `train.txt`: Training data file.
    *   Format: Each pair of lines forms a training sample.
        ```
        Prompt line 1
        Response line 1
        Prompt line 2
        Response line 2
        ...
        ```
*   `input.txt`: Input prompt for the generation phase. The model will be primed with this text.
*   `output.txt`: Generated text will be written here.
*   `param.bin` (if `PARAM_PATH` were used for saving/loading): Would store the best model parameters. Currently not implemented.

## Building

You'll need a C compiler (like GCC) and `pthreads` library.

```bash
gcc -o char_lm main.c -pthread -O2 -Wall
```
*   `-pthread`: Links the pthreads library.
*   `-O2`: Enables optimizations.
*   `-Wall`: Enables common compiler warnings.

## Running

1.  **Prepare `train.txt`:**
    Create a `train.txt` file with prompt-response pairs. For example:
    ```
    hello
    world
    the quick brown fox
    jumps over the lazy dog
    ```
2.  **Prepare `input.txt`:**
    Create an `input.txt` file with the text you want to use as a starting prompt for generation. For example:
    ```
    the cat sat on the
    ```
3.  **Run the executable:**
    ```bash
    ./char_lm
    ```
4.  **Check `output.txt`:**
    After the program finishes training and generation, the generated text will be in `output.txt`.

## Dependencies

*   Standard C Library
*   POSIX Threads (`pthread`)

## Notes

*   The model is quite simple and relies on a brute-force evolutionary search. Performance and quality of generated text will depend heavily on the training data, hyperparameters, and training duration.
*   The `PARAM_BYTESIZE` can become very large, leading to significant memory usage and long training times.
*   Error handling for file operations is present.
*   The random number generator used is `xorshift64`.