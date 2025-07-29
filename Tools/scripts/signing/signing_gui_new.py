#!/usr/bin/env python3

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess
import os
import sys
import threading
import time
import serial.tools.list_ports

class ArduPilotSigningGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Indrones Firmware Signing Tool")
        
        # Create main notebook for tabs
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill='both', expand=True, padx=10, pady=5)
        
        # Create tabs
        self.keys_tab = ttk.Frame(self.notebook)
        self.bootloader_tab = ttk.Frame(self.notebook)
        self.firmware_tab = ttk.Frame(self.notebook)
        self.manage_keys_tab = ttk.Frame(self.notebook)
        
        self.notebook.add(self.keys_tab, text='Generate Keys')
        self.notebook.add(self.bootloader_tab, text='Secure Bootloader')
        self.notebook.add(self.firmware_tab, text='Sign Firmware')
        self.notebook.add(self.manage_keys_tab, text='Manage Keys')
        
        self.setup_keys_tab()
        self.setup_bootloader_tab()
        self.setup_firmware_tab()
        self.setup_manage_keys_tab()
        
        # Status bar
        self.status_var = tk.StringVar()
        self.status_bar = ttk.Label(root, textvariable=self.status_var, relief=tk.SUNKEN)
        self.status_bar.pack(fill=tk.X, side=tk.BOTTOM, padx=5, pady=5)
        
        # Add progress bar and build info frame
        self.progress_frame = ttk.LabelFrame(root, text="Build Progress", padding=10)
        self.progress_frame.pack(fill='x', padx=10, pady=5, before=self.status_bar)
        
        # Progress bar
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(self.progress_frame, 
                                          variable=self.progress_var,
                                          maximum=100.0)
        self.progress_bar.pack(fill='x', pady=5)
        
        # Build output text
        self.build_output = tk.Text(self.progress_frame, height=5, 
                                  font=('Courier', 9))
        self.build_output.pack(fill='x', pady=5)
        self.build_output.config(state='disabled')
        
        # Add scrollbar for build output
        scrollbar = ttk.Scrollbar(self.progress_frame, 
                                command=self.build_output.yview)
        scrollbar.pack(side='right', fill='y')
        self.build_output.config(yscrollcommand=scrollbar.set)
        
        self.set_status("Ready")
        self.progress_var.set(0)

    def setup_keys_tab(self):
        frame = ttk.LabelFrame(self.keys_tab, text="Generate New Key Pair", padding=10)
        frame.pack(fill='x', padx=10, pady=5)
        
        # Key format selection
        format_frame = ttk.Frame(frame)
        format_frame.pack(fill='x', pady=5)
        ttk.Label(format_frame, text="Key Format:").pack(side=tk.LEFT)
        self.key_format = ttk.Combobox(format_frame, 
                                     values=['Monocypher (.dat)', 
                                            'PEM (.pem)', 
                                            'OpenSSL (.key)',
                                            'RSA (.rsa)'])
        self.key_format.pack(side=tk.LEFT, padx=5)
        self.key_format.set('Monocypher (.dat)')
        
        # Name entry
        ttk.Label(frame, text="Vendor/Key Name:").pack(fill='x')
        self.key_name = ttk.Entry(frame)
        self.key_name.pack(fill='x', pady=5)
        
        # Key size selection (for formats that support it)
        size_frame = ttk.Frame(frame)
        size_frame.pack(fill='x', pady=5)
        ttk.Label(size_frame, text="Key Size (bits):").pack(side=tk.LEFT)
        self.key_size = ttk.Combobox(size_frame, 
                                    values=['2048', '3072', '4096'])
        self.key_size.pack(side=tk.LEFT, padx=5)
        self.key_size.set('2048')
        
        # Advanced options
        self.use_advanced = tk.BooleanVar(value=False)
        advanced_check = ttk.Checkbutton(frame, text="Advanced Options", 
                                       variable=self.use_advanced,
                                       command=self.toggle_advanced_options)
        advanced_check.pack(fill='x', pady=5)
        
        # Advanced options frame (hidden by default)
        self.advanced_frame = ttk.Frame(frame)
        ttk.Label(self.advanced_frame, text="Curve Type:").pack(side=tk.LEFT)
        self.curve_type = ttk.Combobox(self.advanced_frame, 
                                      values=['ed25519', 'secp256k1', 'secp384r1'])
        self.curve_type.pack(side=tk.LEFT, padx=5)
        self.curve_type.set('ed25519')
        
        ttk.Button(frame, text="Generate Keys", 
                  command=self.generate_keys).pack(pady=10)

    def toggle_advanced_options(self):
        """Show/hide advanced options based on checkbox state"""
        if self.use_advanced.get():
            self.advanced_frame.pack(fill='x', pady=5)
        else:
            self.advanced_frame.pack_forget()

    def setup_bootloader_tab(self):
        frame = ttk.LabelFrame(self.bootloader_tab, text="Build Secure Bootloader", padding=10)
        frame.pack(fill='x', padx=10, pady=5)
        
        # Board selection with refresh
        board_frame = ttk.Frame(frame)
        board_frame.pack(fill='x', pady=5)
        
        ttk.Label(board_frame, text="Board Name:").pack(side=tk.LEFT)
        self.board_name = ttk.Combobox(board_frame)
        self.board_name.pack(side=tk.LEFT, fill='x', expand=True, padx=5)
        
        ttk.Button(board_frame, text="↻", width=3,
                  command=lambda: self.update_board_list([self.board_name, self.fw_board_name, self.base_board])).pack(side=tk.RIGHT)
        
        # Public key selection
        ttk.Label(frame, text="Public Key File:").pack(fill='x')
        key_frame = ttk.Frame(frame)
        key_frame.pack(fill='x', pady=5)
        
        self.public_key_path = ttk.Entry(key_frame)
        self.public_key_path.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(key_frame, text="Browse", 
                  command=lambda: self.browse_file(self.public_key_path, 
                                                [("Public Key", "*.dat")])).pack(side=tk.RIGHT, padx=5)
        
        self.omit_ardupilot_keys = tk.BooleanVar()
        ttk.Checkbutton(frame, text="Omit ArduPilot Keys", 
                       variable=self.omit_ardupilot_keys).pack(pady=5)
        
        ttk.Button(frame, text="Build Bootloader", 
                  command=self.build_bootloader).pack(pady=10)

    def setup_firmware_tab(self):
        notebook = ttk.Notebook(self.firmware_tab)
        notebook.pack(fill='both', expand=True, padx=5, pady=5)
        
        # Create tab for building firmware
        build_frame = ttk.Frame(notebook)
        notebook.add(build_frame, text='Build & Sign Firmware')
        
        # Create tab for custom board creation
        create_frame = ttk.Frame(notebook)
        notebook.add(create_frame, text='Create Custom Board')
        
        self.setup_build_frame(build_frame)
        self.setup_create_frame(create_frame)

    def setup_build_frame(self, parent):
        frame = ttk.LabelFrame(parent, text="Build and Sign Firmware", padding=10)
        frame.pack(fill='both', expand=True, padx=5, pady=5)

        # Device selection
        device_frame = ttk.LabelFrame(frame, text="Upload Device", padding=10)
        device_frame.pack(fill='x', pady=5)
        
        ttk.Label(device_frame, text="Serial Port:").pack(fill='x')
        port_frame = ttk.Frame(device_frame)
        port_frame.pack(fill='x', pady=5)
        
        self.upload_port_combobox = ttk.Combobox(port_frame)
        self.upload_port_combobox.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(port_frame, text="↻", width=3,
                  command=self.refresh_upload_ports).pack(side=tk.RIGHT, padx=5)
        
        # Board selection with refresh
        board_frame = ttk.Frame(frame)
        board_frame.pack(fill='x', pady=5)
        
        ttk.Label(board_frame, text="Board Name:").pack(side=tk.LEFT)
        self.fw_board_name = ttk.Combobox(board_frame)
        self.fw_board_name.pack(side=tk.LEFT, fill='x', expand=True, padx=5)
        
        ttk.Button(board_frame, text="↻", width=3,
                  command=lambda: self.update_board_list([self.board_name, self.fw_board_name, self.base_board])).pack(side=tk.RIGHT)
        
        # Vehicle selection
        ttk.Label(frame, text="Vehicle Type:").pack(fill='x')
        self.vehicle_type = ttk.Combobox(frame, 
                                       values=['copter', 'plane', 'rover', 'sub', 'tracker'])
        self.vehicle_type.pack(fill='x', pady=5)
        
        # Private key selection
        ttk.Label(frame, text="Private Key File:").pack(fill='x')
        key_frame = ttk.Frame(frame)
        key_frame.pack(fill='x', pady=5)
        
        self.private_key_path = ttk.Entry(key_frame)
        self.private_key_path.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(key_frame, text="Browse", 
                  command=lambda: self.browse_file(self.private_key_path, 
                                                [("Private Key", "*.dat")])).pack(side=tk.RIGHT, padx=5)
        
        # Action buttons
        button_frame = ttk.Frame(frame)
        button_frame.pack(fill='x', pady=10)
        
        ttk.Button(button_frame, text="Build and Sign", 
                  command=self.build_sign_firmware).pack(side=tk.LEFT, expand=True, padx=5)
        ttk.Button(button_frame, text="Upload Firmware", 
                  command=self.upload_firmware).pack(side=tk.LEFT, expand=True, padx=5)

    def setup_create_frame(self, parent):
        frame = ttk.LabelFrame(parent, text="Create Custom Board", padding=10)
        frame.pack(fill='both', expand=True, padx=5, pady=5)
        
        # Base board selection with refresh
        board_frame = ttk.Frame(frame)
        board_frame.pack(fill='x', pady=5)
        
        ttk.Label(board_frame, text="Base Board:").pack(side=tk.LEFT)
        self.base_board = ttk.Combobox(board_frame)
        self.base_board.pack(side=tk.LEFT, fill='x', expand=True, padx=5)
        
        ttk.Button(board_frame, text="↻", width=3,
                  command=lambda: self.update_board_list([self.board_name, self.fw_board_name, self.base_board])).pack(side=tk.RIGHT)
        
        # Custom name entry
        ttk.Label(frame, text="Custom Board Name:").pack(fill='x')
        name_frame = ttk.Frame(frame)
        name_frame.pack(fill='x', pady=5)
        self.custom_name = ttk.Entry(name_frame)
        self.custom_name.pack(side=tk.LEFT, fill='x', expand=True)
        
        # Custom firmware string
        ttk.Label(frame, text="Firmware String:").pack(fill='x')
        self.fw_string = ttk.Entry(frame)
        self.fw_string.pack(fill='x', pady=5)
        
        ttk.Button(frame, text="Create Custom Board", 
                  command=self.create_custom_board).pack(pady=10)

    def setup_manage_keys_tab(self):
        frame = ttk.LabelFrame(self.manage_keys_tab, text="Revert to Normal Boot", padding=10)
        frame.pack(fill='x', padx=10, pady=5)
        
        # Instructions
        ttk.Label(frame, text="This process will remove secure boot and revert to normal boot mode.", 
                 wraplength=400).pack(pady=5)
        
        # Device selection
        device_frame = ttk.LabelFrame(frame, text="Device Connection", padding=10)
        device_frame.pack(fill='x', pady=5)
        
        ttk.Label(device_frame, text="Serial Port:").pack(fill='x')
        port_frame = ttk.Frame(device_frame)
        port_frame.pack(fill='x', pady=5)
        
        self.port_combobox = ttk.Combobox(port_frame)
        self.port_combobox.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(port_frame, text="↻", width=3,
                  command=self.refresh_ports).pack(side=tk.RIGHT, padx=5)
        
        self.refresh_ports()
        
        # Private key selection
        ttk.Label(frame, text="Private Key File:").pack(fill='x')
        key_frame = ttk.Frame(frame)
        key_frame.pack(fill='x', pady=5)
        
        self.manage_private_key = ttk.Entry(key_frame)
        self.manage_private_key.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(key_frame, text="Browse", 
                  command=lambda: self.browse_file(self.manage_private_key, 
                                                [("Private Key", "*.dat")])).pack(side=tk.RIGHT, padx=5)
                                                
        # Board selection
        ttk.Label(frame, text="Board Name:").pack(fill='x')
        board_frame = ttk.Frame(frame)
        board_frame.pack(fill='x', pady=5)
        
        self.manage_board_name = ttk.Combobox(board_frame)
        self.manage_board_name.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(board_frame, text="↻", width=3,
                  command=lambda: self.update_board_list([self.manage_board_name])).pack(side=tk.RIGHT)
        
        # Update board list
        self.update_board_list([self.manage_board_name])

        # Progress section
        progress_frame = ttk.LabelFrame(frame, text="Progress", padding=10)
        progress_frame.pack(fill='x', pady=10)
        
        # Step indicator
        self.revert_step = tk.StringVar(value="Ready to start")
        self.step_label = ttk.Label(progress_frame, textvariable=self.revert_step)
        self.step_label.pack(pady=5)
        
        # Progress indicators (hidden but kept for compatibility)
        self.revert_status = {
            'keys_removed': tk.BooleanVar(value=False),
            'firmware_built': tk.BooleanVar(value=False),
            'bootloader_flashed': tk.BooleanVar(value=False)
        }
        
        # Revert button
        self.revert_button = ttk.Button(frame, 
                                      text="Start Revert Process", 
                                      command=self.revert_to_normal_boot)
        self.revert_button.pack(pady=10)

    def revert_to_normal_boot(self):
        """Handle the complete process of reverting to normal boot"""
        # Get and validate all required fields
        port = self.port_combobox.get().strip()
        private_key = self.manage_private_key.get().strip()
        board = self.manage_board_name.get().strip()  # Using board name from manage keys tab

        # Validate port
        if not port:
            messagebox.showerror("Error", "Please select a serial port")
            return
            
        # Validate private key file
        if not private_key or not os.path.exists(private_key):
            messagebox.showerror("Error", "Please select a valid private key file")
            return
            
        # Validate board selection
        if not board:
            messagebox.showerror("Error", "Please select a board name")
            return
        
        # Now confirm with the user
        if not messagebox.askyesno("Confirm Revert", 
                                  "This will remove secure boot and revert to normal boot mode.\n"
                                  "This process cannot be undone.\n\n"
                                  "Do you want to continue?"):
            return

        def revert_process():
            try:
                # Clear previous output
                self.build_output.config(state='normal')
                self.build_output.delete(1.0, tk.END)
                self.build_output.config(state='disabled')
                
                # Step 1: Remove public keys
                self.revert_step.set("Step 1: Initializing MAVProxy...")
                
                # Initialize MAVProxy with SecureCommand module and get number of keys
                init_cmds = [
                    "module load SecureCommand",
                    f"securecommand set private_keyfile {private_key}",
                    "securecommand getsessionkey",
                    "securecommand getpublickeys"
                ]
                
                # Run initial commands
                output = self.run_mavproxy_commands(port, init_cmds)
                self.revert_step.set("Step 1: Getting number of public keys...")
                
                # Parse number of keys
                num_keys = self.parse_num_keys(output)
                if num_keys > 0:
                    self.update_progress(40, f"Found {num_keys} public keys to remove...")
                    
                    # Remove keys command
                    remove_cmd = [f"securecommand removepublickeys 0 {num_keys}"]
                    self.run_mavproxy_commands(port, remove_cmd)
                    
                    # Verify keys were removed
                    verify_cmd = ["securecommand getpublickeys"]
                    verify_output = self.run_mavproxy_commands(port, verify_cmd)
                    remaining_keys = self.parse_num_keys(verify_output)
                    
                    if remaining_keys == 0:
                        self.update_progress(50, "Successfully removed all public keys!")
                        self.revert_status['keys_removed'].set(True)
                    else:
                        raise Exception(f"Failed to remove all keys. {remaining_keys} keys remaining.")
                
                # Step 2: Build normal firmware with --signed-fw
                self.revert_step.set("Step 2: Building normal firmware...")
                self.update_progress(60, "Configuring build...")
                
                script_dir = os.path.dirname(os.path.abspath(__file__))
                ardupilot_root = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))
                
                subprocess.run([
                    "./waf", "configure", 
                    f"--board={board}", 
                    "--signed-fw"
                ], check=True, cwd=ardupilot_root)
                
                self.update_progress(70, "Building firmware...")
                subprocess.run([
                    "./waf", "copter"
                ], check=True, cwd=ardupilot_root)
                self.revert_status['firmware_built'].set(True)
                
                # Step 3: Flash normal bootloader
                self.revert_step.set("Step 3: Flashing normal bootloader...")
                self.update_progress(90, "Flashing bootloader...")
                self.run_mavproxy_commands(port, ["flashbootloader"])
                self.revert_status['bootloader_flashed'].set(True)
                
                self.update_progress(100, "Successfully reverted to normal boot!")
                messagebox.showinfo("Success", 
                                  "Successfully reverted to normal boot mode.\n"
                                  "You can now use unsigned firmware.")
                
            except subprocess.CalledProcessError as e:
                self.update_progress(0, f"Error: {e.stderr if hasattr(e, 'stderr') else str(e)}")
                messagebox.showerror("Error", f"Failed to revert: {e.stderr if hasattr(e, 'stderr') else str(e)}")
            except Exception as e:
                self.update_progress(0, f"Error: {str(e)}")
                messagebox.showerror("Error", f"Failed to revert: {str(e)}")
            finally:
                self.revert_step.set("Process Complete")
                self.revert_button.configure(state='normal')
                
        # Disable the revert button while processing
        self.revert_button.configure(state='disabled')
        thread = threading.Thread(target=revert_process)
        thread.start()

    def run_mavproxy_commands(self, port, commands):
        """Run a series of MAVProxy commands and return output"""
        # Update progress to show connection attempt
        self.update_progress(10, f"Connecting to MAVProxy on {port}...")

        # Start MAVProxy with minimal modules and no map
        try:
            # First check if the port exists
            if not os.path.exists(port):
                raise Exception(f"Port {port} does not exist")

            # Try to connect with retries
            max_retries = 3
            retry_count = 0
            process = None

            while retry_count < max_retries:
                try:
                    process = subprocess.Popen(
                        ["mavproxy.py", "--master", port, 
                         "--streamrate=1", "--moddebug=3",  # Reduced stream rate
                         "--non-interactive", "--norc",
                         "--aircraft=revert"],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        stdin=subprocess.PIPE,
                        text=True,
                        bufsize=1,
                        universal_newlines=True
                    )
                    
                    # Wait for initialization and check if process is still running
                    time.sleep(3)
                    if process.poll() is None:
                        # Process is still running
                        break
                    else:
                        raise Exception("MAVProxy process terminated unexpectedly")
                        
                except Exception as e:
                    retry_count += 1
                    if retry_count == max_retries:
                        raise Exception(f"Failed to start MAVProxy after {max_retries} attempts: {str(e)}")
                    self.update_progress(10, f"Connection attempt {retry_count} failed, retrying...")
                    time.sleep(2)
            
            if process is None:
                raise Exception("Failed to start MAVProxy process")

            output_lines = []
            # Additional wait for stable connection
            time.sleep(2)
            
            # Load required module first
            process.stdin.write("module load SecureCommand\n")
            process.stdin.flush()
            time.sleep(1)
            
            # Execute each command with proper delays
            progress = 20
            for cmd in commands:
                if cmd == "module load SecureCommand":
                    continue  # Skip as we've already loaded it
                    
                self.update_progress(progress, f"Executing: {cmd}")
                try:
                    # Check if process is still running
                    if process.poll() is not None:
                        raise Exception("MAVProxy process terminated unexpectedly")
                        
                    process.stdin.write(cmd + "\n")
                    process.stdin.flush()
                    time.sleep(3)  # Increased delay for command completion
                    
                    # Read output with timeout
                    start_time = time.time()
                    timeout = 10  # 10 seconds timeout for each command
                except Exception as e:
                    raise Exception(f"Failed to execute command {cmd}: {str(e)}")
                
                while (time.time() - start_time) < timeout:
                    line = process.stdout.readline()
                    if not line:
                        time.sleep(0.1)  # Small delay to prevent CPU spinning
                        continue
                        
                    line = line.strip()
                    if line:
                        output_lines.append(line)
                        self.update_progress(progress, f"Output: {line}")
                        
                        # Check for command completion markers
                        if any(marker in line for marker in [
                            "SECURE_COMMAND_RESULT received",
                            "Number of public keys:",
                            "Keys removed successfully",
                            "Command failed",
                            "Command succeeded"
                        ]):
                            break
                            
                progress += 10
            # Clean exit
            process.stdin.write("exit\n")
            process.stdin.flush()
            time.sleep(1)
            
            self.update_progress(100, "Commands completed successfully")
            return "\n".join(output_lines)
            
        except Exception as e:
            self.update_progress(0, f"Error executing MAVProxy commands: {str(e)}")
            raise
        finally:
            # Ensure process is terminated
            try:
                # Try graceful shutdown first
                if process.poll() is None:
                    process.stdin.write("exit\n")
                    process.stdin.flush()
                    time.sleep(1)
                    
                # If process is still running, terminate it
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=2)
                    
                # If still running, force kill
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=1)
            except Exception as e:
                print(f"Error during cleanup: {str(e)}")
                # Ignore cleanup errors and continue  # Process already dead

    def parse_num_keys(self, output):
        """Parse number of public keys from MAVProxy output"""
        for line in output.splitlines():
            if "Number of public keys:" in line:
                return int(line.split(":")[1].strip())
        return 0

    def refresh_ports(self):
        """Refresh the list of available serial ports"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combobox['values'] = ports
        if ports:
            self.port_combobox.set(ports[0])

    def refresh_upload_ports(self):
        """Refresh the list of available serial ports for firmware upload"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.upload_port_combobox['values'] = ports
        if ports:
            self.upload_port_combobox.set(ports[0])

    def connect_device(self):
        """Connect to the selected device"""
        port = self.port_combobox.get()
        if not port:
            messagebox.showerror("Error", "Please select a serial port")
            return
        
        # Enable the key management buttons
        for child in self.key_buttons_frame.winfo_children():
            child.configure(state='normal')
        
        messagebox.showinfo("Success", f"Connected to device on {port}")

    def set_status(self, message):
        self.status_var.set(message)
        self.root.update_idletasks()

    def browse_file(self, entry_widget, file_types):
        filename = filedialog.askopenfilename(filetypes=file_types)
        if filename:
            entry_widget.delete(0, tk.END)
            entry_widget.insert(0, filename)

    def run_command(self, cmd, success_msg):
        def run():
            try:
                self.set_status("Running command...")
                print(f"Running command: {' '.join(cmd)}")
                result = subprocess.run(cmd, capture_output=True, text=True, check=True)
                print(f"Output: {result.stdout}")
                messagebox.showinfo("Success", success_msg)
                self.set_status("Ready")
            except subprocess.CalledProcessError as e:
                error_msg = f"Command failed:\nCommand: {' '.join(cmd)}\nError: {e.stderr}\nOutput: {e.stdout}"
                print(error_msg)
                messagebox.showerror("Error", error_msg)
                self.set_status("Error occurred")
            except Exception as e:
                error_msg = f"Exception occurred:\nCommand: {' '.join(cmd)}\nError: {str(e)}"
                print(error_msg)
                messagebox.showerror("Error", error_msg)
                self.set_status("Error occurred")

        thread = threading.Thread(target=run)
        thread.start()

    def update_board_list(self, comboboxes):
        """Update the board list in multiple comboboxes"""
        boards = []
        
        # Get standard boards from hwdef directory
        script_dir = os.path.dirname(os.path.abspath(__file__))
        ardupilot_root = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))
        hwdef_dir = os.path.join(ardupilot_root, 'libraries', 'AP_HAL_ChibiOS', 'hwdef')
        
        if os.path.exists(hwdef_dir):
            boards = [d for d in os.listdir(hwdef_dir) 
                     if os.path.isdir(os.path.join(hwdef_dir, d)) and 
                     os.path.exists(os.path.join(hwdef_dir, d, 'hwdef.dat'))]
        
        for combobox in comboboxes:
            combobox['values'] = boards

    def generate_keys(self):
        """Generate keys using selected format and options"""
        name = self.key_name.get().strip()
        if not name:
            messagebox.showerror("Error", "Please enter a vendor/key name")
            return
        
        key_format = self.key_format.get().split(' ')[0].lower()  # Get first word in lowercase
        script_dir = os.path.dirname(os.path.abspath(__file__))
        
        try:
            if key_format == 'monocypher':
                # Use existing monocypher key generation
                generate_keys_path = os.path.join(script_dir, "generate_keys.py")
                cmd = [sys.executable, generate_keys_path, name]
            else:
                # Use OpenSSL for other formats
                key_size = self.key_size.get()
                curve = self.curve_type.get() if self.use_advanced.get() else 'ed25519'
                
                if key_format == 'pem':
                    cmd = ["openssl", "genpkey", "-algorithm", "ED25519" if curve == 'ed25519' else "RSA",
                          "-out", f"{name}_private.pem"]
                    if curve != 'ed25519':
                        cmd.extend(["-pkeyopt", f"rsa_keygen_bits:{key_size}"])
                elif key_format == 'openssl':
                    cmd = ["openssl", "genpkey", "-algorithm", curve.upper(),
                          "-out", f"{name}_private.key"]
                elif key_format == 'rsa':
                    cmd = ["openssl", "genrsa", "-out", f"{name}_private.rsa", key_size]
                
                # Generate public key
                if key_format != 'monocypher':
                    self.run_command(cmd, "Private key generated")
                    pub_cmd = ["openssl", "pkey", "-in", f"{name}_private.{key_format}",
                             "-pubout", "-out", f"{name}_public.{key_format}"]
                    self.run_command(pub_cmd, f"Keys generated successfully:\n{name}_private.{key_format}\n{name}_public.{key_format}")
                    return
            
            self.run_command(cmd, f"Keys generated successfully:\n{name}_private_key.dat\n{name}_public_key.dat")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to generate keys: {str(e)}")
            self.set_status("Error occurred")

    def build_bootloader(self):
        board = self.board_name.get().strip()
        pub_key = self.public_key_path.get().strip()
        
        if not board or not pub_key:
            messagebox.showerror("Error", "Please fill in all fields")
            return
        
        def build():
            try:
                # Clear previous build output
                self.build_output.config(state='normal')
                self.build_output.delete(1.0, tk.END)
                self.build_output.config(state='disabled')
                self.progress_var.set(0)
                
                # Start bootloader build
                self.set_status("Building secure bootloader...")
                self.update_progress(10, f"Building bootloader for {board}...")
                
                cmd = ["./Tools/scripts/build_bootloaders.py", board, f"--signing-key={pub_key}"]
                if self.omit_ardupilot_keys.get():
                    cmd.append("--omit-ardupilot-keys")
                    self.update_progress(20, "Omitting ArduPilot keys from bootloader...")
                
                # Run bootloader build
                self.update_progress(30, f"Running command: {' '.join(cmd)}")
                result = subprocess.run(cmd, capture_output=True, text=True, check=True)
                
                # Process output in chunks for better progress indication
                output_lines = result.stdout.split('\n')
                total_lines = len(output_lines)
                for i, line in enumerate(output_lines):
                    if line.strip():
                        progress = 30 + (i / total_lines * 60)  # Progress from 30% to 90%
                        self.update_progress(progress, line)
                
                self.update_progress(100, "Bootloader built successfully!")
                messagebox.showinfo("Success", "Bootloader built successfully")
                self.set_status("Ready")
                
            except subprocess.CalledProcessError as e:
                self.update_progress(0, f"Build failed:\n{e.stderr}")
                messagebox.showerror("Error", f"Bootloader build failed:\n{e.stderr}")
                self.set_status("Error occurred")
            except Exception as e:
                self.update_progress(0, f"Error: {str(e)}")
                messagebox.showerror("Error", str(e))
                self.set_status("Error occurred")

        thread = threading.Thread(target=build)
        thread.start()

    def update_progress(self, value, message=None):
        """Update progress bar and optionally add message to build output"""
        self.progress_var.set(value)
        if message:
            self.build_output.config(state='normal')
            self.build_output.insert('end', message + '\n')
            self.build_output.see('end')
            self.build_output.config(state='disabled')
        self.root.update_idletasks()

    def build_sign_firmware(self):
        board = self.fw_board_name.get().strip()
        vehicle = self.vehicle_type.get()
        priv_key = self.private_key_path.get().strip()
        
        if not all([board, vehicle, priv_key]):
            messagebox.showerror("Error", "Please fill in all fields")
            return
        
        def build():
            try:
                # Clear previous build output
                self.build_output.config(state='normal')
                self.build_output.delete(1.0, tk.END)
                self.build_output.config(state='disabled')
                self.progress_var.set(0)
                
                # Configure build
                self.set_status("Configuring build...")
                self.update_progress(10, "Configuring build with signing enabled...")
                result = subprocess.run(["./waf", "configure", 
                                       f"--board={board}", "--signed-fw"], 
                                     capture_output=True, text=True, check=True)
                self.update_progress(30, result.stdout)
                
                # Build firmware
                self.set_status("Building firmware...")
                self.update_progress(40, f"Building {vehicle} firmware...")
                result = subprocess.run(["./waf", vehicle], 
                                     capture_output=True, text=True, check=True)
                self.update_progress(70, result.stdout)
                
                # Sign firmware
                self.set_status("Signing firmware...")
                self.update_progress(80, "Signing firmware with provided key...")
                result = subprocess.run(["./Tools/scripts/signing/make_secure_fw.py",
                                     f"build/{board}/bin/ardu{vehicle}.apj",
                                     priv_key], 
                                    capture_output=True, text=True, check=True)
                self.update_progress(100, "Build and signing completed successfully!")
                
                messagebox.showinfo("Success", "Firmware built and signed successfully")
                self.set_status("Ready")
                
            except subprocess.CalledProcessError as e:
                self.update_progress(0, f"Build failed:\n{e.stderr}")
                messagebox.showerror("Error", f"Build failed:\n{e.stderr}")
                self.set_status("Error occurred")
            except Exception as e:
                self.update_progress(0, f"Error: {str(e)}")
                messagebox.showerror("Error", str(e))
                self.set_status("Error occurred")

        thread = threading.Thread(target=build)
        thread.start()

    def upload_firmware(self):
        """Upload the signed firmware to the device and handle bootloader flashing"""
        port = self.upload_port_combobox.get()
        board = self.fw_board_name.get().strip()
        vehicle = self.vehicle_type.get()
        
        if not all([port, board, vehicle]):
            messagebox.showerror("Error", "Please select device port, board name and vehicle type")
            return
            
        try:
            # Check if the board has secure bootloader using multiple methods
            self.set_status("Checking bootloader status...")
            has_secure = False
            
            # Method 1: Check USB device name
            try:
                port_info = next((port for port in serial.tools.list_ports.comports() 
                                if port.device == port), None)
                if port_info and "ArduPilot_Secure" in port_info.description:
                    has_secure = True
            except Exception as e:
                print(f"Warning: Could not check USB device name: {str(e)}")

            # Method 2: Check dmesg output
            if not has_secure:
                try:
                    # Get recent dmesg entries
                    dmesg_result = subprocess.run(["dmesg", "--since=1m"], 
                                               capture_output=True, text=True, check=True)
                    if "ArduPilot_Secure" in dmesg_result.stdout:
                        has_secure = True
                except Exception as e:
                    print(f"Warning: Could not check dmesg output: {str(e)}")

            if has_secure:
                messagebox.showinfo("Info", "Device has secure bootloader enabled")
            else:
                if messagebox.askyesno("Warning", 
                                     "Secure bootloader not detected. Continue anyway?"):
                    pass
                else:
                    return
            
            # Upload firmware
            firmware_path = f"build/{board}/bin/ardu{vehicle}.apj"
            if not os.path.exists(firmware_path):
                messagebox.showerror("Error", "Firmware file not found. Please build the firmware first.")
                return
                
            cmd = ["./Tools/scripts/uploader.py",
                   "--port", port,
                   firmware_path]
                   
            self.set_status(f"Uploading firmware to {port}...")
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            # After firmware upload, ask about flashing bootloader
            if messagebox.askyesno("Flash Bootloader", 
                                 "Firmware uploaded successfully! Would you like to flash the bootloader now?"):
                self.set_status("Flashing bootloader...")
                mavproxy_cmd = ["mavproxy.py", "--master", port,
                              "--cmd", "flashbootloader"]
                try:
                    subprocess.run(mavproxy_cmd, capture_output=True, text=True, check=True)
                    messagebox.showinfo("Success", "Bootloader flashed successfully!")
                except subprocess.CalledProcessError as e:
                    error_msg = f"Bootloader flash failed:\n{e.stderr}"
                    messagebox.showerror("Error", error_msg)
            
            self.set_status("Ready")
            
        except subprocess.CalledProcessError as e:
            error_msg = f"Upload failed:\n{e.stderr}"
            messagebox.showerror("Error", error_msg)
            self.set_status("Upload failed")
        except Exception as e:
            error_msg = f"Upload failed: {str(e)}"
            messagebox.showerror("Error", error_msg)
            self.set_status("Upload failed")

    def create_custom_board(self):
        base_board = self.base_board.get().strip()
        custom_name = self.custom_name.get().strip()
        fw_string = self.fw_string.get().strip()
        
        if not all([base_board, custom_name, fw_string]):
            messagebox.showerror("Error", "Please fill in all fields")
            return
        
        if not custom_name.startswith('Indrones'):
            custom_name = 'Indrones' + custom_name
        
        try:
            script_dir = os.path.dirname(os.path.abspath(__file__))
            ardupilot_root = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))
            
            hwdef_dir = os.path.join(ardupilot_root, 'libraries', 'AP_HAL_ChibiOS', 'hwdef')
            custom_dir = os.path.join(hwdef_dir, custom_name)
            os.makedirs(custom_dir, exist_ok=True)
            
            # Create hwdef.dat
            with open(os.path.join(custom_dir, 'hwdef.dat'), 'w') as f:
                f.write(f'include ../{base_board}/hwdef.dat\n')
                f.write(f'define AP_CUSTOM_FIRMWARE_STRING "{fw_string}"\n')
            
            # Copy required files
            files_to_copy = ['hwdef-bl.dat', 'hwdef-bl.inc', 'hwdef.inc', 
                           'defaults.parm', 'README.md', 'sdcard.inc']
            base_dir = os.path.join(hwdef_dir, base_board)
            for file in files_to_copy:
                src = os.path.join(base_dir, file)
                dst = os.path.join(custom_dir, file)
                if os.path.exists(src):
                    import shutil
                    shutil.copy2(src, dst)
            
            os.makedirs(os.path.join(custom_dir, 'scripts'), exist_ok=True)
            
            # Update README.md
            with open(os.path.join(custom_dir, 'README.md'), 'w') as f:
                f.write(f'# {custom_name}\n\n')
                f.write(f'Custom board based on {base_board}\n\n')
                f.write(f'Firmware string: {fw_string}\n\n')
                f.write('## Customizations\n\n')
                f.write('- Initial creation\n')
            
            # Build bootloader
            self.set_status(f"Building bootloader for {custom_name}...")
            bootloader_cmd = ["./Tools/scripts/build_bootloaders.py", custom_name]
            try:
                subprocess.run(bootloader_cmd, cwd=ardupilot_root, check=True, capture_output=True)
                messagebox.showinfo("Success", f"Custom board {custom_name} created successfully and bootloader built")
            except subprocess.CalledProcessError as e:
                error_msg = f"Board created but bootloader build failed:\n{e.stderr.decode()}"
                messagebox.showerror("Warning", error_msg)
            
            self.update_board_list([self.board_name, self.fw_board_name, self.base_board])
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to create custom board: {str(e)}")
            return

    def get_session_key(self):
        port = self.port_combobox.get()
        priv_key = self.manage_private_key.get().strip()
        if not priv_key:
            messagebox.showerror("Error", "Please select a private key file")
            return
        
        cmd = ["mavproxy.py", "--master", port,
               "--cmd", f"module load SecureCommand; securecommand set private_keyfile {priv_key}; securecommand getsessionkey"]
        self.run_command(cmd, "Session key obtained successfully")

    def list_public_keys(self):
        port = self.port_combobox.get()
        cmd = ["mavproxy.py", "--master", port,
               "--cmd", "securecommand getpublickeys"]
        self.run_command(cmd, "Public keys listed successfully")
    
    def remove_public_keys(self):
        if not messagebox.askyesno("Confirm", "Are you sure you want to remove all public keys? This cannot be undone."):
            return
            
        port = self.port_combobox.get()
        cmd = ["mavproxy.py", "--master", port,
               "--cmd", "securecommand getpublickeys; securecommand removepublickeys 0 4"]
        self.run_command(cmd, "Public keys removed successfully")

if __name__ == "__main__":
    root = tk.Tk()
    app = ArduPilotSigningGUI(root)
    root.mainloop()
