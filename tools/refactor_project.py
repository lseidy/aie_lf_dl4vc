import re
import os
import fnmatch
import argparse
import shutil
from tempfile import mkstemp
from pathlib import Path

def copy_with_directory_structure(src_files, dest, start):
    output_files = []
    for f in src_files:
        relpath = os.path.relpath(f, start=start)
        output_filepath = os.path.join(dest, relpath)
        os.makedirs(os.path.dirname(output_filepath), exist_ok=True)
        actual_output_filepath = shutil.copy2(f, output_filepath, follow_symlinks=True)
        output_files.append(actual_output_filepath)

        print(f"{f} => {actual_output_filepath}")

    return output_files


def get_paths(start_dir, patterns):
    p = Path(start_dir)
    if not isinstance(patterns, list):
        patterns = [patterns]

    for pattern in patterns:
        for path in p.glob(pattern):
            if path.is_file():
                yield str(path.resolve())

def pretty_print(li):
    for s in li:
        print(s)


def refactor_files(src_files, refactor_func):
    for filepath in src_files:
        fd, tmpfile = mkstemp(prefix=os.path.basename(filepath))
        with open(filepath) as f, open(tmpfile, 'w') as fout:
            for line in f.readlines():
                line = refactor_func(line)
                fout.writelines([line])
        
        shutil.move(tmpfile, filepath)
        print(f"Refactored: {filepath}")

def refactor_files_v2(src_files, refactor_func):
    for filepath in src_files:
        fd, tmpfile = mkstemp(prefix=os.path.basename(filepath))
        with open(filepath) as f, open(tmpfile, 'w') as fout:
            whole_content = f.read()
            whole_content = refactor_func(whole_content)
            fout.write(whole_content)
        
        shutil.move(tmpfile, filepath)
        print(f"Refactored: {filepath}")

def rename_filenames(src_files, file_rename_func):
    new_filepaths = []
    for filepath in src_files:
        dirname = os.path.dirname(filepath) 
        basename = os.path.basename(filepath) 
        dest_basename = file_rename_func(basename)
        outpath = os.path.join(dirname, dest_basename)
        shutil.move(filepath, outpath)
        new_filepaths.append(outpath)

        print(f"Renamed: {filepath} -> {outpath}")
    
    return new_filepaths

def main(args):
    input_directory = args.input_directory
    output_directory = args.output_directory
    new_name = args.new_name
    project_name = args.project_name
    readme_content = args.readme_content
    introduction_footer = "This project is started from Evey [https://gitlab.com/openalgo/evey](https://gitlab.com/openalgo/evey)"

    os.makedirs(output_directory, exist_ok=True)

    output_directory = os.path.abspath(output_directory)
    input_directory = os.path.abspath(input_directory)

    files_to_copy = {}
    files_to_refactor = {}
    readme_to_refactor = {}
    files_to_rename = {}

    
    ## Get all file paths

    # Find CMakeLists.txt
    cmake_files = list(get_paths(input_directory, ["**/CMakeLists.txt"]))
    files_to_copy['cmake'] = cmake_files
    files_to_refactor['cmake'] = cmake_files

    # Find *.c *.h
    src_files = list(get_paths(input_directory, ["**/*.[ch]"]))
    files_to_copy['src'] = src_files
    files_to_refactor['src'] = src_files
    files_to_rename['src'] = src_files

    # Readme file
    readme_files = list(get_paths(input_directory, ["README.md"]))
    files_to_copy['readme'] = readme_files
    files_to_refactor['readme'] = readme_files
    readme_to_refactor['readme'] = readme_files

    
    # Misc files
    misc_files = list(get_paths(input_directory, ["**/.git*"]))
    files_to_copy['misc'] = misc_files
    files_to_refactor['misc'] = misc_files
    
    # Ignore .git, .vscode, this script file
    ignore_files = list(get_paths(input_directory, ["**/.git/**/*", "**/.vscode/**/*", "**/tools/refactor_project.py"]))
    if __file__ and __file__ not in ignore_files:
        ignore_files.append(__file__)

    # All files (only copied)
    all_files = list(get_paths(input_directory, "**/*"))
    everything_else = list(set(all_files) - set(cmake_files + src_files + misc_files + readme_files + ignore_files))
    files_to_copy['everything_else'] = everything_else
    
    
    # Copy files to output directory
    print("1. Cloning files")
    for name, filepaths in files_to_copy.items():
        output_filepaths = copy_with_directory_structure(filepaths, output_directory, input_directory)

        filepaths.clear()
        filepaths.extend(output_filepaths)
    

    ## Refactor content of files
    print("2. Refactor file contents")
    def refactor_func(line):
        # Define more stages here to modify the line
        if project_name is not None and new_name is not None:
            line = re.sub(project_name.lower(), new_name.lower(), line)
            line = re.sub(project_name.title(), new_name.title(), line)
            line = re.sub(project_name.upper(), new_name.upper(), line)
        return line
    
    for name, filepaths in files_to_refactor.items():
        refactor_files(filepaths, refactor_func)
    
    def readme_refactor_func(content):
        content = re.sub("(?P<fl>^\s*#[^#]*?\n)[^#]*##", f"\g<fl>\n{readme_content}\n\n{introduction_footer}\n\n##", content)
        return content
    
    for name, filepaths in readme_to_refactor.items():
        refactor_files_v2(filepaths, readme_refactor_func)


    ## Refactor names of files
    def file_rename_func(filename):
        # Define more stages here to modify the line
        if project_name is not None and new_name is not None:
            filename = re.sub(project_name.lower(), new_name.lower(), filename)
            filename = re.sub(project_name.title(), new_name.title(), filename)
            filename = re.sub(project_name.upper(), new_name.upper(), filename)
        return filename
    
    print("3. Renaming files")
    for name, filepaths in files_to_rename.items():
        renamed_filepaths = rename_filenames(filepaths, file_rename_func)

        filepaths.clear()
        filepaths.extend(renamed_filepaths)

    def tot_len(d: dict):
        return sum([len(v) for v in d.values()])

    print(f"Total no. of files in src: \t\t\t {len(all_files)}")
    print(f"Total no. of files ignored: \t\t\t {len(ignore_files)}")
    print(f"Total no. of files copied: \t\t\t {tot_len(files_to_copy)}")
    print(f"No. of files refactored: \t\t\t {tot_len(files_to_refactor)}")
    print(f"No. of files renamed: \t\t\t\t {tot_len(files_to_rename)}")
    print(f"Output directory: \t\t\t\t {output_directory}")
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-o', '--output-directory', required=True, type=str, help='Output project directory. Note: should not be inside the `project-directory`')
    parser.add_argument('-n', '--new-name', type=str, required=True, help='The project name to rename **to**')
    parser.add_argument('-i', '--input-directory', default="../", type=str, help='Input project source directory')
    parser.add_argument('-p', '--project-name', type=str, default="evey", help='The project name to rename **from**')
    parser.add_argument('--readme-content', type=str, default="", help='content to add to the Readme file')

    args = parser.parse_args()

    main(args)
