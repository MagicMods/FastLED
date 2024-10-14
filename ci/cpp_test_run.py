import argparse
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass

from ci.paths import PROJECT_ROOT


@dataclass
class FailedTest:
    name: str
    return_code: int
    stdout: str
    stderr: str


def run_command(command, use_gdb=False) -> tuple[int, str, str]:
    if use_gdb:
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as gdb_script:
            gdb_script.write("set pagination off\n")
            gdb_script.write("run\n")
            gdb_script.write("bt full\n")
            gdb_script.write("info registers\n")
            gdb_script.write("x/16i $pc\n")
            gdb_script.write("thread apply all bt full\n")
            gdb_script.write("quit\n")

        gdb_command = f"gdb -return-child-result -batch -x {gdb_script.name} --args {command}"
        process = subprocess.Popen(
            gdb_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
            text=True,
        )
        stdout, stderr = process.communicate()
        os.unlink(gdb_script.name)
        return process.returncode, stdout, stderr
    else:
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
            text=True,
        )
        stdout, stderr = process.communicate()
        return process.returncode, stdout, stderr


def compile_tests(clean: bool = False, unknown_args: list[str] = []) -> None:
    os.chdir(str(PROJECT_ROOT))
    print("Compiling tests...")
    command = ["uv", "run", "ci/cpp_test_compile.py"]
    if clean:
        command.append("--clean")
    command.extend(unknown_args)
    return_code, stdout, stderr = run_command(" ".join(command))
    if return_code != 0:
        print("Compilation failed:")
        print(stdout)
        print(stderr)
        sys.exit(1)
    print(stdout)
    print(stderr)
    print("Compilation successful.")


def run_tests() -> None:
    test_dir = os.path.join("tests", ".build", "bin")
    if not os.path.exists(test_dir):
        print(f"Test directory not found: {test_dir}")
        sys.exit(1)

    print("Running tests...")
    failed_tests: list[FailedTest] = []
    files = os.listdir(test_dir)
    # filter out all pdb files (windows)
    files = [f for f in files if not f.endswith(".pdb")]
    for test_file in files:
        test_path = os.path.join(test_dir, test_file)
        if os.path.isfile(test_path) and os.access(test_path, os.X_OK):
            print(f"Running test: {test_file}")
            return_code, stdout, stderr = run_command(test_path)

            if return_code != 0:
                print("Test failed. Re-running with GDB to get stack trace...")
                _, gdb_stdout, gdb_stderr = run_command(test_path, use_gdb=True)
                stdout += "\n--- GDB Output ---\n" + gdb_stdout
                stderr += "\n--- GDB Errors ---\n" + gdb_stderr

                # Extract crash information
                crash_info = extract_crash_info(gdb_stdout)
                if crash_info:
                    print(f"Crash occurred at: {crash_info.get('file', 'Unknown')}:{crash_info.get('line', 'Unknown')}")
                    if 'cause' in crash_info:
                        print(f"Cause: {crash_info['cause']}")
                    if 'stack' in crash_info:
                        print(f"Stack: {crash_info['stack']}")

            print("Test output:")
            print(stdout)
            if stderr:
                print("Test errors:")
                print(stderr)
            print(
                f"Test {'passed' if return_code == 0 else 'failed'} with return code {return_code}"
            )
            print("-" * 40)
            if return_code != 0:
                failed_tests.append(FailedTest(test_file, return_code, stdout, stderr))
    if failed_tests:
        for failed_test in failed_tests:
            print(
                f"Test {failed_test.name} failed with return code {failed_test.return_code}\n{failed_test.stdout}"
            )
        tests_failed = len(failed_tests)
        failed_test_names = [test.name for test in failed_tests]
        print(
            f"{tests_failed} test{'s' if tests_failed != 1 else ''} failed: {', '.join(failed_test_names)}"
        )
        sys.exit(1)
    print("All tests passed.")

def extract_crash_info(gdb_output: str) -> dict:
    lines = gdb_output.split('\n')
    crash_info = {}
    for i, line in enumerate(lines):
        if line.startswith('Program received signal'):
            crash_info['cause'] = line.split(':', 1)[1].strip()
        elif line.startswith('#0'):
            # Found the crash point
            crash_info['stack'] = line
            for j in range(i, len(lines)):
                if 'at' in lines[j]:
                    parts = lines[j].split('at')
                    if len(parts) == 2:
                        file_line = parts[1].strip()
                        file, line = file_line.rsplit(':', 1)
                        crash_info['file'] = file.strip()
                        crash_info['line'] = line.strip()
                        return crash_info
    return crash_info


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compile and run C++ tests")
    parser.add_argument(
        "--compile-only",
        action="store_true",
        help="Only compile the tests without running them",
    )
    parser.add_argument(
        "--run-only",
        action="store_true",
        help="Only run the tests without compiling them",
    )
    parser.add_argument(
        "--clean", action="store_true", help="Clean build before compiling"
    )
    args, unknown = parser.parse_known_args()
    args.unknown = unknown
    return args


def main() -> None:
    args = parse_args()

    if not args.run_only:
        compile_tests(clean=args.clean, unknown_args=args.unknown)

    if not args.compile_only:
        run_tests()


if __name__ == "__main__":
    main()
