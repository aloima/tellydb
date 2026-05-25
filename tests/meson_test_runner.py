import subprocess
import time
import sys
import os

def main():
    # The first argument should be the path to the telly binary
    if len(sys.argv) < 2:
        print("Usage: meson_test_runner.py <telly_binary_path>")
        sys.exit(1)

    telly_bin = sys.argv[1]

    print(f"Starting telly server: {telly_bin}")
    server_process = subprocess.Popen([telly_bin], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    time.sleep(1.5)

    print("Running pytest...")
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(tests_dir)

    try:
        result = subprocess.run(["pytest"], cwd=project_root)
        exit_code = result.returncode
    except Exception as e:
        print(f"Failed to run pytest: {e}")
        exit_code = 1
    finally:
        # Terminate the server
        print("Stopping telly server...")
        server_process.terminate()
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_process.kill()

    sys.exit(exit_code)

if __name__ == "__main__":
    main()
