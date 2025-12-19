import unittest
import redis
from time import sleep


class AgeCommand(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.client = redis.Redis(host="localhost", port=6379, db=0)

    def test_without_arguments(self):
        response = self.client.execute_command("AGE")
        self.assertIsInstance(response, int)

    def test_valid(self):
        before = self.client.execute_command("AGE")
        sleep(2)
        after = self.client.execute_command("AGE")

        self.assertEqual(after - before, 2)

    def test_with_arguments(self):
        response = self.client.execute_command("AGE", "a", "b")
        self.assertIsInstance(response, int)
