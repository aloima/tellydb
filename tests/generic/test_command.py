import unittest
import redis
from redis.exceptions import ResponseError
import logging

import sys
from pathlib import Path

constants_path = Path(__file__).resolve().parent.parent
sys.path.append(str(constants_path))

try:
    from constants import invalid_subcommand, missing_subcommand
except ImportError:
    pass


class CommandCommand(unittest.TestCase):
    client_2: redis.Redis
    client_3: redis.Redis

    @classmethod
    def setUpClass(self):
        self.client_2 = redis.Redis(host="localhost", port=6379, protocol=2)
        self.client_3 = redis.Redis(host="localhost", port=6379, protocol=3)

    def test_list_subcommand(self):
        response = self.client_2.execute_command("COMMAND LIST")
        self.assertIsInstance(response, list)

        for element in response:
            self.assertIsInstance(element, bytes)
            self.assertTrue(element.isalpha())

    def test_count_subcommand(self):
        response = self.client_2.execute_command("COMMAND COUNT")
        self.assertIsInstance(response, int)

    def test_missing_subcommand(self):
        with self.assertRaises(ResponseError) as e:
            self.client_2.execute_command("COMMAND")

        self.assertEqual(e.exception.args[0], missing_subcommand("COMMAND"))

    def test_invalid_subcommand(self):
        with self.assertRaises(ResponseError) as e:
            self.client_2.execute_command("COMMAND INVALID")

        self.assertEqual(e.exception.args[0], invalid_subcommand("COMMAND"))

    def test_docs_subcommand(self):
        response = self.client_2.execute_command("COMMAND DOCS")
        response_3 = self.client_3.execute_command("COMMAND DOCS")
        logging.warning(response_3)
        self.assertIsInstance(response, list)
        self.assertEqual(len(response) % 2, 0)

        response = iter(response)
        for name, data in zip(response, response):
            self.assertIsInstance(name, bytes)
            self.assertTrue(name.isalpha())
            self.assertIsInstance(data, list)

            keys = {b"summary", b"since", b"complexity"}

            data = iter(data)
            for key, value in zip(data, data):
                self.assertIsInstance(key, bytes)
                self.assertTrue(key.isalpha())

                self.assertIn(key, keys)
                keys.remove(key)

                self.assertIsInstance(value, bytes)
                self.assertTrue(all(32 <= b <= 126 for b in value))
