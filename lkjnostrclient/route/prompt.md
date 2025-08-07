**Act as an expert frontend developer specializing in functional programming and the Nostr protocol.** Your task is to generate a single, self-contained HTML file that functions as a simple Nostr client.

Follow these requirements and the step-by-step plan meticulously.

**Core Requirements:**

1.  **Library:** Use `nostr-tools`. Import it from the unpkg CDN: `https://unpkg.com/nostr-tools/lib/nostr.bundle.js`. All functions will be available under the `window.NostrTools` global object.
2.  **Relays:** Connect to a pool of 5 popular, public relays.
    *   `wss://relay.damus.io`
    *   `wss://yabu.me`
    *   `wss://nos.lol`
    *   `wss://nostr.wine`
    *   `wss://relay.primal.net`
3.  **Authentication:** Do **not** implement NIP-07 browser extension login. The user will manage their secret key by pasting an `nsec` string into a text input.
4.  **UI Layout:**
    *   The screen must be split into two vertical columns.
    *   **Left Column (Controls):**
        *   A textarea for composing a new note.
        *   A text input for the user's secret key (`nsec`).
        *   A "Generate New Keys" button.
        *   A "Post" button.
        *   An area to display the newly generated or current public key (`npub`).
    *   **Right Column (View):**
        *   A timeline that displays `kind: 1` notes from the relays in chronological order (newest first).
5.  **Programming Paradigm:** Strictly adhere to a **functional programming** style.
    *   Use pure functions wherever possible.
    *   Isolate side effects (like DOM manipulation and network requests).
    *   Avoid mutating global state directly.
6.  **Error Handling:** Implement robust error handling using a **Rust-inspired `Result` type**. Create simple `Ok(value)` and `Err(error)` factory functions to represent success and failure states, and handle them explicitly.
7.  **Code Structure:**
    *   **Small Functions:** Decompose the logic into small, single-responsibility functions.
    *   **No Phantom Functions:** **This is critical.** Do not call any functions or access any properties that are not explicitly defined in the provided `nostr-tools` documentation. Be extremely careful with function signatures.
8.  **Timeline Subscription:** Pay special attention to the timeline subscription. Use the correct method and handle incoming events properly to update the UI. Ensure new notes are prepended, not appended, to the timeline.

---

**Crucial `nostr-tools` API Guidance (from documentation):**

To avoid common mistakes, please adhere to the following `nostr-tools` usage patterns, which will be accessed via `window.NostrTools`.

*   **Key Generation & Handling:**
    *   `NostrTools.generateSecretKey()`: Returns a `Uint8Array`.
    *   `NostrTools.getPublicKey(sk: Uint8Array)`: Takes a `Uint8Array` secret key and returns a hex-encoded public key string.
    *   **Input Handling:** The user will provide an `nsec` string. Use `NostrTools.nip19.decode(nsecString)` to get a `{type, data}` object. The `data` property will be the `Uint8Array` secret key required by other functions. Your code must handle this decoding.
    *   **Encoding:** Use `NostrTools.nip19.nsecEncode(sk: Uint8Array)` and `NostrTools.nip19.npubEncode(pk: string)` to display keys to the user.
*   **Event Creation:**
    *   `NostrTools.finalizeEvent(eventTemplate: object, sk: Uint8Array)`: Signs an event and returns the final event object. The second argument **must** be a `Uint8Array`.
*   **Relay Interaction with `SimplePool`:**
    *   Instantiate with `const pool = new NostrTools.SimplePool()`.
    *   **Publishing:** `pool.publish(relays: string[], event: object)`: Publishes a signed event to the specified relays. Use `Promise.any()` to handle the array of promises it returns.
    *   **Subscribing (for Timeline):** Use `pool.subscribe()` for a real-time feed. **Do not use `pool.get()` or `pool.querySync()` for the timeline, as they are not suitable for continuous updates.**
        *   The correct signature is `sub = pool.subscribe(relays: string[], filters: object, callbacks: object)`.
        *   `filters` is an **object**, e.g., `{ kinds: [1], limit: 100 }`.
        *   The `callbacks` object should have an `onevent` property: `{ onevent(event) { /* ... UI update logic ... */ } }`.
        *   Keep a reference to the subscription object (`sub`) returned by `pool.subscribe` so you can call `sub.unsubscribe()` when a new subscription is created.

---

**Step-by-Step Implementation Plan:**

Please structure your code generation process by thinking through these steps.

1.  **Step 1: HTML and CSS Foundation:**
    *   Create the basic HTML document structure (`<!DOCTYPE html>`, `head`, `body`).
    *   Include the `nostr-tools` script tag in the `<head>`.
    *   Add `<style>` tags with CSS for the two-column layout, inputs, buttons, and a visually clear timeline display.
2.  **Step 2: Setup and Initial State:**
    *   Define the constant array of relay URLs.
    *   Initialize `const pool = new NostrTools.SimplePool()`.
    *   Define a global state object `let appState = { currentSubscription: null };` to manage the active subscription.
3.  **Step 3: Functional Utilities:**
    *   Create the `Result` type helper functions: `const Ok = (value) => ({ ok: true, value });` and `const Err = (error) => ({ ok: false, error });`.
    *   Create pure helper functions for DOM element selection, e.g., `const $ = (selector) => document.querySelector(selector);`.
4.  **Step 4: Key Management Functions:**
    *   `generateNewKeys()`: A pure function that returns an object with `sk` (Uint8Array), `nsec` (string), and `npub` (string).
    *   `getSecretKeyFromInput(nsecValue)`: A pure function that takes the `nsec` string, attempts to decode it using `NostrTools.nip19.decode`, and returns a `Result<Uint8Array, string>`. It must validate that the decoded type is 'nsec'.
5.  **Step 5: Event Publishing Functions:**
    *   `createNoteEvent(content, sk)`: A pure function that takes text content and a secret key (`Uint8Array`), and returns a finalized (signed) `kind: 1` event object.
    *   `publishEvent(pool, relays, event)`: An impure function that takes the pool, relays, and a signed event, and uses `pool.publish()` to send it. It should return a `Promise` that resolves on success or rejects on failure.
6.  **Step 6: Timeline Subscription Functions:**
    *   `renderNote(event)`: A pure function that takes a Nostr event object, creates a corresponding HTML element (e.g., a `<div>` with content, author pubkey, and creation date), and returns the DOM element. Make it robust against malformed event content.
    *   `subscribeToTimeline(pool, relays, timelineContainer)`: A function that performs the side effect of subscribing. It should:
        *   Define the subscription filter: `[{ kinds: [1], limit: 50 }]`.
        *   Call `pool.subscribe()` with the relays, filter, and an `onevent` callback.
        *   The callback should use `renderNote` to create an element and **prepend** it to the `timelineContainer`.
        *   Return the subscription object so it can be managed.
7.  **Step 7: UI Update Functions (Impure Boundary):**
    *   `updateKeyUI(nsec, npub)`: Updates the input fields and display areas with key data.
    *   `displayError(message)`: Shows an error to the user (e.g., via `alert()`).
    *   `clearTimeline()`: Clears all child nodes from the timeline container.
8.  **Step 8: Main Logic and Event Listeners (The Impure Root):**
    *   Create a main `main` function that runs on `DOMContentLoaded`.
    *   Inside `main`, get all necessary DOM elements.
    *   Attach a `click` listener to the "Generate New Keys" button. This handler should call `generateNewKeys` and then `updateKeyUI` with the results.
    *   Attach a `click` listener to the "Post" button. This handler should orchestrate the process:
        1.  Read the nsec from the input.
        2.  Call `getSecretKeyFromInput`. If `Err`, `displayError`.
        3.  If `Ok`, get content from the textarea. If empty, `displayError`.
        4.  Call `createNoteEvent`.
        5.  Call `publishEvent`. Handle the promise, showing success/error messages.
    *   Attach a `blur` event listener to the secret key input. This handler should be the primary trigger for changing the timeline subscription:
        1.  If `appState.currentSubscription` exists, call `appState.currentSubscription.unsubscribe()`.
        2.  Call `clearTimeline()`.
        3.  Read the nsec from input. Call `getSecretKeyFromInput`.
        4.  If `Ok`, call `subscribeToTimeline` and store the result in `appState.currentSubscription`. Also, derive and display the `npub`.
        5.  If `Err`, do nothing or display a subtle error.

**Final Output:**

Please provide the complete and final code as a single HTML file within a markdown code block. Ensure the code is well-commented, explaining the purpose of each function and how the functional and imperative parts are separated, as requested in the step-by-step plan.