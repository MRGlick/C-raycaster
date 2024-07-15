import os

def delete_bmp_files(folder):
    for root, dirs, files in os.walk(folder):
        for file in files:
            if file.endswith(".bmp"):
                file_path = os.path.join(root, file)
                try:
                    os.remove(file_path)
                    print(f"Deleted: {file_path}")
                except Exception as e:
                    print(f"Error deleting {file_path}: {e}")

if __name__ == "__main__":
    folder_path = input("Enter the path of the folder: ")
    delete_bmp_files(folder_path)
    print("Deletion complete.")