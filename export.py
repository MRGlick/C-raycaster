# Thanks GPT!

import os
import shutil
import zipfile


# Define the names of the folders and files to be copied
folders_to_copy = ["Sounds", "Textures", "Levels", "Shaders"]
files_to_copy = ["SDL2.dll", "SDL2_ttf.dll", "SDL2_gpu.dll", "handcannon_multiplayer.exe", "FFFFORWA.TTF", "run_as_server.bat"]

# Define the name of the export folder and the zip file
export_folder = "export"
zip_filename = "export.zip"

# Ensure the export folder doesn't already exist
if os.path.exists(export_folder):
    shutil.rmtree(export_folder)

# Create the export folder
os.mkdir(export_folder)

# Copy the specified folders into the export folder
for folder in folders_to_copy:
    if os.path.exists(folder):
        shutil.copytree(folder, os.path.join(export_folder, folder))

# Copy the specified files into the export folder
for file in files_to_copy:
    if os.path.exists(file):
        shutil.copy(file, export_folder)

# Zip the export folder
with zipfile.ZipFile(zip_filename, 'w') as zipf:
    for root, dirs, files in os.walk(export_folder):
        for file in files:
            file_path = os.path.join(root, file)
            arcname = os.path.relpath(file_path, export_folder)
            zipf.write(file_path, arcname)

# Remove the export folder after zipping
shutil.rmtree(export_folder)

print(f"Export completed successfully. {zip_filename} created.")