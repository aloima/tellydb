import unittest
from tellypy import Client, Kind, Protocol

import sys
from pathlib import Path

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import invalid_subcommand, missing_subcommand
except ImportError:
    pass


class CommandCommand(unittest.TestCase):
    client_2: Client
    client_3: Client

    @classmethod
    def setUpClass(self):
        self.client_2 = Client(host="localhost", port=6379)
        self.client_2.connect()

        self.client_3 = Client(host="localhost", port=6379)
        self.client_3.connect(protocol=Protocol.RESP3)

    def test_list_subcommand(self):
        response = self.client_2.execute_command("COMMAND LIST")
        self.assertEqual(response.kind, Kind.ARRAY)
        self.assertIsInstance(response.data, list)

        for element in response.data:
            self.assertEqual(element.kind, Kind.SIMPLE_STRING)
            self.assertIsInstance(element.data, str)
            self.assertTrue(element.data.isalpha())

    def test_count_subcommand(self):
        response = self.client_2.execute_command("COMMAND COUNT")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)

    def test_missing_subcommand(self):
        response = self.client_2.execute_command("COMMAND")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, missing_subcommand("COMMAND"))

    def test_invalid_subcommand(self):
        response = self.client_2.execute_command("COMMAND INVALID")
        self.assertEqual(response.kind, Kind.SIMPLE_ERROR)
        self.assertIsInstance(response.data, str)
        self.assertEqual(response.data, invalid_subcommand("COMMAND"))

    def test_docs_subcommand(self):
        response = self.client_2.execute_command("COMMAND DOCS")
        self.assertEqual(response.kind, Kind.ARRAY)
        self.assertIsInstance(response.data, list)
        self.assertEqual(len(response.data) % 2, 0)

        response = iter(response.data)
        for name, info in zip(response, response):
            self.assertEqual(name.kind, Kind.BULK_STRING)
            self.assertIsInstance(name.data, str)
            self.assertTrue(name.data.isalpha())

            self.assertEqual(info.kind, Kind.ARRAY)
            self.assertIsInstance(info.data, list)
            self.assertGreater(len(info.data), 0)

            keys = {"summary", "since", "complexity"}

            info = iter(info.data)
            for key, value in zip(info, info):
                self.assertEqual(key.kind, Kind.BULK_STRING)
                self.assertIsInstance(key.data, str)
                self.assertTrue(key.data.isalpha())

                self.assertIn(key.data, keys)
                keys.remove(key.data)

                self.assertIsInstance(value.data, str)
                self.assertTrue(all(32 <= ord(b) <= 126 for b in value.data))

        response = self.client_3.execute_command("COMMAND DOCS")
        self.assertEqual(response.kind, Kind.MAP)
        self.assertIsInstance(response.data, dict)

        for name, info in response.data.items():
            self.assertEqual(name.kind, Kind.BULK_STRING)
            self.assertIsInstance(name.data, str)
            self.assertTrue(name.data.isalpha())

            self.assertEqual(info.kind, Kind.MAP)
            self.assertIsInstance(info.data, dict)
            self.assertGreater(len(info.data), 0)

            keys = {"summary", "since", "complexity"}

            for key, value in info.data.items():
                self.assertEqual(key.kind, Kind.BULK_STRING)
                self.assertIsInstance(key.data, str)
                self.assertTrue(key.data.isalpha())

                self.assertIn(key.data, keys)
                keys.remove(key.data)

                self.assertIsInstance(value.data, str)
                self.assertTrue(all(32 <= ord(b) <= 126 for b in value.data))
