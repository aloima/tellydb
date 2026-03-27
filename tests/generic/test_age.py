import unittest
from time import sleep

from tellypy import Client, Kind


class AgeCommand(unittest.TestCase):
    client: Client

    @classmethod
    def setUpClass(cls):
        cls.client = Client(host="localhost", port=6379)
        cls.client.connect()

    def test_without_arguments(self):
        response = self.client.execute_command("AGE")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)

    def test_valid(self):
        before = self.client.execute_command("AGE").data
        sleep(2)
        after = self.client.execute_command("AGE").data

        self.assertEqual(after - before, 2)

    def test_with_arguments(self):
        response = self.client.execute_command("AGE a b")
        self.assertEqual(response.kind, Kind.INTEGER)
        self.assertIsInstance(response.data, int)
