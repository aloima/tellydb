import unittest
import redis


class ClientCommand(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.client = redis.Redis(host="localhost", port=6379, db=0)

    def test_id_argument(self):
        response = self.client.execute_command("CLIENT", "ID")
        self.assertIsInstance(response, int)
        self.assertGreater(response, 0)
