#!/usr/bin/env python3

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess
import os
import sys
import threading

class ArduPilotSigningGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ArduPilot Firmware Signing Tool")
        
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
        
        self.set_status("Ready")

    def setup_keys_tab(self):
        # Key Generation Frame
        frame = ttk.LabelFrame(self.keys_tab, text="Generate New Key Pair", padding=10)
        frame.pack(fill='x', padx=10, pady=5)
        
        ttk.Label(frame, text="Vendor/Key Name:").pack(fill='x')
        self.key_name = ttk.Entry(frame)
        self.key_name.pack(fill='x', pady=5)
        
        ttk.Button(frame, text="Generate Keys", 
                  command=self.generate_keys).pack(pady=10)

    def setup_bootloader_tab(self):
        frame = ttk.LabelFrame(self.bootloader_tab, text="Build Secure Bootloader", padding=10)
        frame.pack(fill='x', padx=10, pady=5)
        
        ttk.Label(frame, text="Board Name:").pack(fill='x')
        self.board_name = ttk.Combobox(frame)
        self.board_name.pack(fill='x', pady=5)
        ttk.Button(frame, text="Refresh Board List", 
                  command=lambda: self.update_board_list([self.board_name, self.fw_board_name])).pack(pady=5)
        
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
        
    def setup_build_frame(self, frame):
        """Setup the build and sign firmware frame"""
        # Board selection for building
        ttk.Label(frame, text="Board Name:").pack(fill='x', padx=10, pady=5)
        self.fw_board_name = ttk.Combobox(frame)
        self.fw_board_name.pack(fill='x', padx=10, pady=5)
        ttk.Button(frame, text="Refresh Board List", 
                  command=lambda: self.update_board_list([self.board_name, self.fw_board_name])).pack(padx=10, pady=5)
        
        # Base board selection
        ttk.Label(custom_frame, text="Base Board:").pack(fill='x')
        self.base_board = ttk.Combobox(custom_frame, 
                                    values=['CubeOrange', 'Pixhawk4', 'Pixhawk6X', 'CubeYellow'])
        self.base_board.pack(fill='x', pady=5)
        
        # Custom name entry
        ttk.Label(custom_frame, text="Custom Board Name:").pack(fill='x')
        name_frame = ttk.Frame(custom_frame)
        name_frame.pack(fill='x', pady=5)
        self.custom_name = ttk.Entry(name_frame)
        self.custom_name.pack(side=tk.LEFT, fill='x', expand=True)
        
        # Custom firmware string
        ttk.Label(custom_frame, text="Firmware String:").pack(fill='x')
        self.fw_string = ttk.Entry(custom_frame)
        self.fw_string.pack(fill='x', pady=5)
        
        ttk.Button(custom_frame, text="Create Custom Board", 
                  command=self.create_custom_board).pack(pady=5)
        
        # Board selection for building
        ttk.Label(frame, text="Board Name:").pack(fill='x')
        self.fw_board_name = ttk.Combobox(frame)
        self.fw_board_name.pack(fill='x', pady=5)
        self.update_board_list()
        
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
        
        ttk.Button(frame, text="Build and Sign Firmware", 
                  command=self.build_sign_firmware).pack(pady=10)

    def setup_manage_keys_tab(self):
        frame = ttk.LabelFrame(self.manage_keys_tab, text="Manage Public Keys", padding=10)
        frame.pack(fill='x', padx=10, pady=5)
        
        # Connection settings
        ttk.Label(frame, text="Private Key File:").pack(fill='x')
        key_frame = ttk.Frame(frame)
        key_frame.pack(fill='x', pady=5)
        
        self.manage_private_key = ttk.Entry(key_frame)
        self.manage_private_key.pack(side=tk.LEFT, fill='x', expand=True)
        
        ttk.Button(key_frame, text="Browse", 
                  command=lambda: self.browse_file(self.manage_private_key, 
                                                [("Private Key", "*.dat")])).pack(side=tk.RIGHT, padx=5)
        
        # Buttons for key management
        ttk.Button(frame, text="Get Session Key", 
                  command=self.get_session_key).pack(pady=5)
        ttk.Button(frame, text="List Public Keys", 
                  command=self.list_public_keys).pack(pady=5)
        ttk.Button(frame, text="Remove All Public Keys", 
                  command=self.remove_public_keys).pack(pady=5)

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
                print(f"Running command: {' '.join(cmd)}")  # Print the command being run
                result = subprocess.run(cmd, capture_output=True, text=True, check=True)
                print(f"Output: {result.stdout}")  # Print stdout for debugging
                messagebox.showinfo("Success", success_msg)
                self.set_status("Ready")
            except subprocess.CalledProcessError as e:
                error_msg = f"Command failed:\nCommand: {' '.join(cmd)}\nError: {e.stderr}\nOutput: {e.stdout}"
                print(error_msg)  # Print to console for debugging
                messagebox.showerror("Error", error_msg)
                self.set_status("Error occurred")
            except Exception as e:
                error_msg = f"Exception occurred:\nCommand: {' '.join(cmd)}\nError: {str(e)}"
                print(error_msg)  # Print to console for debugging
                messagebox.showerror("Error", error_msg)
                self.set_status("Error occurred")

        thread = threading.Thread(target=run)
        thread.start()

    def generate_keys(self):
        name = self.key_name.get().strip()
        if not name:
            messagebox.showerror("Error", "Please enter a vendor/key name")
            return
        
        # Get the absolute path to generate_keys.py
        script_dir = os.path.dirname(os.path.abspath(__file__))
        generate_keys_path = os.path.join(script_dir, "generate_keys.py")
        
        cmd = [sys.executable, generate_keys_path, name]
        self.run_command(cmd, f"Keys generated successfully:\n{name}_private_key.dat\n{name}_public_key.dat")

    def build_bootloader(self):
        board = self.board_name.get().strip()
        pub_key = self.public_key_path.get().strip()
        
        if not board or not pub_key:
            messagebox.showerror("Error", "Please fill in all fields")
            return
        
        cmd = ["Tools/scripts/build_bootloaders.py", board, f"--signing-key={pub_key}"]
        if self.omit_ardupilot_keys.get():
            cmd.append("--omit-ardupilot-keys")
        
        self.run_command(cmd, "Bootloader built successfully")

    def build_sign_firmware(self):
        board = self.fw_board_name.get().strip()
        vehicle = self.vehicle_type.get()
        priv_key = self.private_key_path.get().strip()
        
        if not all([board, vehicle, priv_key]):
            messagebox.showerror("Error", "Please fill in all fields")
            return
        
        def build():
            try:
                self.set_status("Configuring build...")
                subprocess.run(["./waf", "configure", f"--board={board}", "--signed-fw"], 
                            check=True, capture_output=True)
                
                self.set_status("Building firmware...")
                subprocess.run(["./waf", vehicle], check=True, capture_output=True)
                
                self.set_status("Signing firmware...")
                subprocess.run(["./Tools/scripts/signing/make_secure_fw.py",
                            f"build/{board}/bin/ardu{vehicle}.apj",
                            priv_key], check=True, capture_output=True)
                
                messagebox.showinfo("Success", "Firmware built and signed successfully")
                self.set_status("Ready")
                
            except subprocess.CalledProcessError as e:
                messagebox.showerror("Error", f"Build failed:\n{e.stderr}")
                self.set_status("Error occurred")
            except Exception as e:
                messagebox.showerror("Error", str(e))
                self.set_status("Error occurred")

        thread = threading.Thread(target=build)
        thread.start()

    def get_session_key(self):
        priv_key = self.manage_private_key.get().strip()
        if not priv_key:
            messagebox.showerror("Error", "Please select a private key file")
            return
        
        cmd = ["mavproxy.py", "--cmd", f"module load SecureCommand; securecommand set private_keyfile {priv_key}; securecommand getsessionkey"]
        self.run_command(cmd, "Session key obtained successfully")

    def list_public_keys(self):
        cmd = ["mavproxy.py", "--cmd", "securecommand getpublickeys"]
        self.run_command(cmd, "Public keys listed successfully")

    def remove_public_keys(self):
        if messagebox.askyesno("Confirm", "Are you sure you want to remove all public keys? This cannot be undone."):
            cmd = ["mavproxy.py", "--cmd", "securecommand getpublickeys; securecommand removepublickeys 0 4"]
            self.run_command(cmd, "Public keys removed successfully")

    def update_board_list(self):
        """Update the board list combobox with standard and custom boards"""
        boards = ['CubeOrange', 'Pixhawk4', 'Pixhawk6X', 'CubeYellow', 
                 'KakuteH7', 'MatekF405', 'omnibusf4pro', 'KakuteF4']
        
        # Add custom boards from hwdef directory in main ArduPilot libraries
        script_dir = os.path.dirname(os.path.abspath(__file__))
        ardupilot_root = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))
        hwdef_dir = os.path.join(ardupilot_root, 'libraries', 'AP_HAL_ChibiOS', 'hwdef')
        if os.path.exists(hwdef_dir):
            custom_boards = [d for d in os.listdir(hwdef_dir) 
                           if os.path.isdir(os.path.join(hwdef_dir, d)) and d.startswith('Indrones')]
            boards.extend(custom_boards)
        
        self.fw_board_name['values'] = boards

    def create_custom_board(self):
        """Create a custom board based on an existing board"""
        base_board = self.base_board.get().strip()
        custom_name = self.custom_name.get().strip()
        fw_string = self.fw_string.get().strip()
        
        if not all([base_board, custom_name, fw_string]):
            messagebox.showerror("Error", "Please fill in all fields")
            return
        
        if not custom_name.startswith('Indrones'):
            custom_name = 'Indrones' + custom_name
        
        try:
            # Get the ArduPilot root directory
            script_dir = os.path.dirname(os.path.abspath(__file__))
            ardupilot_root = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))
            
            # Create directory structure in the main ArduPilot libraries
            hwdef_dir = os.path.join(ardupilot_root, 'libraries', 'AP_HAL_ChibiOS', 'hwdef')
            custom_dir = os.path.join(hwdef_dir, custom_name)
            os.makedirs(custom_dir, exist_ok=True)
            
            # Create hwdef.dat
            with open(os.path.join(custom_dir, 'hwdef.dat'), 'w') as f:
                f.write(f'include ../{base_board}/hwdef.dat\n')
                f.write(f'define AP_CUSTOM_FIRMWARE_STRING "{fw_string}"\n')
            
            # Copy required files from base board
            files_to_copy = ['hwdef-bl.dat', 'hwdef-bl.inc', 'hwdef.inc', 
                           'defaults.parm', 'README.md', 'sdcard.inc']
            base_dir = os.path.join(hwdef_dir, base_board)
            for file in files_to_copy:
                src = os.path.join(base_dir, file)
                dst = os.path.join(custom_dir, file)
                if os.path.exists(src):
                    import shutil
                    shutil.copy2(src, dst)
            
            # Create scripts directory
            os.makedirs(os.path.join(custom_dir, 'scripts'), exist_ok=True)
            
            # Update README.md
            with open(os.path.join(custom_dir, 'README.md'), 'w') as f:
                f.write(f'# {custom_name}\n\n')
                f.write(f'Custom board based on {base_board}\n\n')
                f.write(f'Firmware string: {fw_string}\n\n')
                f.write('## Customizations\n\n')
                f.write('- Initial creation\n')
            
            # Build bootloader for the custom board
            self.set_status(f"Building bootloader for {custom_name}...")
            bootloader_cmd = ["./Tools/scripts/build_bootloaders.py", custom_name]
            try:
                subprocess.run(bootloader_cmd, cwd=ardupilot_root, check=True, capture_output=True)
                messagebox.showinfo("Success", f"Custom board {custom_name} created successfully and bootloader built")
            except subprocess.CalledProcessError as e:
                error_msg = f"Board created but bootloader build failed:\n{e.stderr.decode()}"
                messagebox.showerror("Warning", error_msg)
            
            self.update_board_list()
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to create custom board: {str(e)}")
            return

if __name__ == "__main__":
    root = tk.Tk()
    app = ArduPilotSigningGUI(root)
    root.mainloop()
