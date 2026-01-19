OUT_OF_BOUNDS = "Specified ID is out of bounds for uint32_t"


def wrong_argument(command: str):
    return f"Wrong argument count for '{command}' command"


def invalid_subcommand(command: str):
    return f"Invalid subcommand for '{command}' command"


def missing_subcommand(command: str):
    return f"Missing subcommand for '{command}' command"
