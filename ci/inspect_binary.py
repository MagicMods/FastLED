import argparse
import json
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
PROJECT_ROOT = HERE.parent


def cpp_filt(cpp_filt_path: str | Path, input_text: str) -> str:
    p = Path(cpp_filt_path)
    if not p.exists():
        raise FileNotFoundError(f"cppfilt not found at '{p}'")
    command = [str(p), "-t"]
    print(f"Running command: {' '.join(command)}")
    result = subprocess.run(
        command,
        input=input_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"Error running command: {result.stderr}")
    return result.stdout


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Inspect a compiled binary")
    parser.add_argument("--first", action="store_true", help="Inspect the first board")
    parser.add_argument("--cwd", type=str, help="Custom working directory")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.cwd:
        # os.chdir(args.cwd)
        root_build_dir = Path(args.cwd) / ".build"
    else:
        root_build_dir = Path(".build")

    # Find the first board directory
    board_dirs = [d for d in root_build_dir.iterdir() if d.is_dir()]
    if not board_dirs:
        # print("No board directories found in .build")
        print(f"No board directories found in {root_build_dir.absolute()}")
        return 1

    # display all the boards to the user and ask them to select which one they want by number
    print("Available boards:")
    for i, board_dir in enumerate(board_dirs):
        print(f"[{i}]: {board_dir.name}")

    if args.first:
        which = 0
    else:
        which = int(input("Enter the number of the board you want to inspect: "))

    board_dir = board_dirs[which]
    board = board_dir.name

    build_info_json = board_dir / "build_info.json"
    build_info = json.loads(build_info_json.read_text())
    board_info = build_info.get(board) or build_info[next(iter(build_info))]
    firmware_path = Path(board_info["prog_path"])
    cpp_filt_path = Path(board_info["aliases"]["c++filt"])
    map_path = board_dir / "firmware.map"
    if not map_path.exists():
        # Some platforms like esp32 ignore the map path and instead just
        # place it in the same directory as the firmware.
        map_path = firmware_path.with_suffix(".map")
    if map_path.exists():
        print(f"Dumping map file for {board} compiled firmware binary at {map_path}")
        input_text = map_path.read_text()
        demangled_text = cpp_filt(cpp_filt_path, input_text)
        print(demangled_text)
    else:
        print(f"Map file not found at {map_path}")

    return 0


if __name__ == "__main__":
    main()
