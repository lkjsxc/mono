# Note: To use pynput, you need to install the library beforehand.
# pip install pynput
#
# On macOS, you may need to grant control permission to the application running Python (Terminal, IDE, etc.)
# in System Settings > Security & Privacy > Privacy > Accessibility.
# On Linux, additional packages (e.g., python3-xlib or xlib-dev, python3-dev, libudev-dev, libinput-dev)
# may be required depending on your environment.

from pynput.keyboard import Key, Controller as KeyboardController
from pynput.mouse import Button, Controller as MouseController
import time
import re

# --- Global Controllers ---
keyboard = KeyboardController()
mouse = MouseController()

# --- Settings ---
MINECRAFT_ACTIVATE_WAIT = 3
COMMAND_EXEC_WAIT = 0.3
COMMAND_FILE_PATH = "cb.mcfunction"

INITIAL_BASE_X = 0
INITIAL_BASE_Y = 60
INITIAL_BASE_Z = 0

COMMAND_BLOCK_FACING = "up"
COMMAND_BLOCK_CONDITION = "false"
CHAIN_COMMAND_BLOCK_AUTO = "true"

# --- Function Definitions ---

def focus_minecraft_window():
    print(f"Bring the Minecraft window to the foreground within {MINECRAFT_ACTIVATE_WAIT} seconds,")
    print("and ensure it's ready for command input.")
    print("The script will then start processing automatically.")
    time.sleep(MINECRAFT_ACTIVATE_WAIT)

def send_minecraft_command(command_text):
    mouse.press(Button.right)
    mouse.release(Button.right)
    time.sleep(0.4)

    for char_to_type in command_text:
        keyboard.type(char_to_type)
        time.sleep(0.005)
    time.sleep(0.1)

    keyboard.press(Key.enter)
    keyboard.release(Key.enter)
    time.sleep(COMMAND_EXEC_WAIT)

def escape_command_string(cmd_str):
    escaped_str = cmd_str.replace('\\', '\\\\').replace('"', '\\"')
    return escaped_str

def parse_commands_from_file(filepath):
    instructions = []
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line_number, line_content in enumerate(f, 1):
                line_content = line_content.strip()

                if not line_content or line_content.startswith('# comment'):
                    continue

                move_match = re.match(r"# move, *(-?\d+\.?\d*), *(-?\d+\.?\d*), *(-?\d+\.?\d*)", line_content)
                if move_match:
                    try:
                        x = int(float(move_match.group(1)))
                        y = int(float(move_match.group(2)))
                        z = int(float(move_match.group(3)))
                        instructions.append({'type': 'move', 'value': (x, y, z)})
                    except ValueError:
                        print(f"Warning: Invalid coordinates in #move line at file line {line_number}: {line_content}")
                else:
                    instructions.append({'type': 'command', 'value': line_content})
        
    except FileNotFoundError:
        print(f"Error: Command file '{filepath}' not found.")
        return None
    
    return instructions

# --- Main Processing ---
if __name__ == "__main__":
    print("Minecraft Automatic Command Block Placement Script (Repeating: Needs Power / Chain: Always Active, uses pynput)")
    print("Chat open action is set to MOUSE RIGHT CLICK.")

    focus_minecraft_window()
    
    print(f"Command file: {COMMAND_FILE_PATH}")
    
    parsed_instructions = parse_commands_from_file(COMMAND_FILE_PATH)

    if parsed_instructions is None:
        exit()
    
    if not parsed_instructions:
        print("No instructions to process were found in the file.")
        exit()

    current_base_x = INITIAL_BASE_X
    current_base_y = INITIAL_BASE_Y
    current_base_z = INITIAL_BASE_Z
    
    if not (parsed_instructions and parsed_instructions[0]['type'] == 'move'):
        print(f"Starting with initial base coordinates: ({INITIAL_BASE_X}, Y from {INITIAL_BASE_Y}, {INITIAL_BASE_Z}).")
        print("Coordinates will be changed by #move commands in the file.")

    block_counter_overall = 0
    block_counter_current_stack = 0

    try:
        for instruction in parsed_instructions:
            if instruction['type'] == 'move':
                move_x, move_y, move_z = instruction['value']

                # --- Added: Execute /fill command to clear area ---
                # The fill command will use the X and Z from the #move command,
                # and clear from the Y specified in #move up to Y=128.
                if move_y <= 128: # Only fill if the starting Y is not above 128
                    fill_command = f"fill {move_x} {move_y} {move_z} {move_x} 128 {move_z} minecraft:air replace"
                    print(f"Clearing area with command: {fill_command}")
                    send_minecraft_command(fill_command)
                else:
                    print(f"Skipping area clearing for move to ({move_x}, {move_y}, {move_z}) as Y ({move_y}) is above 128.")
                # --- End of added code ---

                current_base_x = move_x
                current_base_y = move_y # This is the starting Y for placing command blocks
                current_base_z = move_z
                
                print(f"\nMoving: New base coordinates for command blocks ({current_base_x}, Y from {current_base_y}, {current_base_z}).")
                block_counter_current_stack = 0
                continue

            elif instruction['type'] == 'command':
                cmd_to_set_in_block = instruction['value']
                
                # Target coordinates for the current command block
                # Note: current_base_y is incremented after each block placement
                target_x = current_base_x
                target_y = current_base_y 
                target_z = current_base_z
                
                block_counter_overall += 1
                block_counter_current_stack += 1

                print(f"Block {block_counter_overall} (current stack {block_counter_current_stack}) @ ({target_x}, {target_y}, {target_z}): \"{cmd_to_set_in_block[:60]}...\"")
                
                escaped_cmd_for_nbt = escape_command_string(cmd_to_set_in_block)
                
                block_id_str = ""
                block_states_str = f"[facing={COMMAND_BLOCK_FACING.lower()},conditional={COMMAND_BLOCK_CONDITION.lower()}]"
                
                auto_setting_str = "" 

                if block_counter_current_stack == 1:
                    block_id_str = "minecraft:repeating_command_block"
                    auto_setting_str = "0b" 
                    print(f" -> Type: Repeating (Facing:{COMMAND_BLOCK_FACING}, Conditional:{COMMAND_BLOCK_CONDITION}, Execution:Needs Power)")
                else:
                    block_id_str = "minecraft:chain_command_block"
                    auto_setting_str = '1b' if CHAIN_COMMAND_BLOCK_AUTO.lower() == 'true' else '0b'
                    chain_auto_text = "Always Active" if CHAIN_COMMAND_BLOCK_AUTO.lower() == 'true' else "Needs Power"
                    print(f" -> Type: Chain (Facing:{COMMAND_BLOCK_FACING}, Conditional:{COMMAND_BLOCK_CONDITION}, Execution:{chain_auto_text})")
                
                nbt_data_str = f"{{Command:\"{escaped_cmd_for_nbt}\",auto:{auto_setting_str}}}"
                
                minecraft_setblock_cmd = (
                    f"setblock {target_x} {target_y} {target_z} "
                    f"{block_id_str}{block_states_str}{nbt_data_str} replace"
                )
                
                send_minecraft_command(minecraft_setblock_cmd)
                
                current_base_y += 1 # Increment Y for the next block in the stack
                
        print("\nAll instructions have been processed.")

    except KeyboardInterrupt:
        print("\nScript interrupted (Ctrl+C).")
    except Exception as e:
        print(f"\nAn error occurred: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("Exiting script.")