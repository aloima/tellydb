import unittest

from tellypy import Client, Kind


OUT_OF_BOUNDS = "Specified ID is out of bounds for uint32_t"


def wrong_argument(command: str):
    return f"Wrong argument count for '{command}' command"


def invalid_subcommand(command: str):
    return f"Invalid subcommand for '{command}' command"


def invalid_type(command: str):
    return f"Invalid type for '{command}' command"


def missing_subcommand(command: str):
    return f"Missing subcommand for '{command}' command"


class ExtendedTestCase(unittest.TestCase):
    client: Client

    def assertSimpleErrorEqual(self, command: str, error: str):
        response = self.client.execute_command(command)
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, error)

    def assertIntegerEqual(self, command: str, value: int):
        response = self.client.execute_command(command)
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
        self.assertEqual(response.data, value)

    def assertSimpleStringEqual(self, command: str, value: str):
        response = self.client.execute_command(command)
        self.assertEqual(response.kind, Kind.SIMPLE_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, value)

    def assertBulkStringEqual(self, command: str, value: str):
        response = self.client.execute_command(command)
        self.assertEqual(response.kind, Kind.BULK_STRING)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, value)

    def assertNullEqual(self, command: str):
        response = self.client.execute_command(command)
        self.assertEqual(response.kind, Kind.NULL)
        self.assertIsInstance(response.data, type(None))
        self.assertEqual(response.data, None)

    def assertBooleanEqual(self, command: str, value: bool):
        response = self.client.execute_command(command)
        self.assertEqual(response.kind, Kind.BOOLEAN)
        self.assertIsInstance(response.data, bool)
        self.assertEqual(response.data, value)
