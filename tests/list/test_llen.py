import sys
from pathlib import Path

from tellypy import Client

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import ExtendedTestCase, invalid_type, wrong_argument
except ImportError:
    sys.exit(1)


class LLenCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(cls):
        cls.client = Client(host="localhost", port=6379)
        cls.client.connect()

        cls.client.execute_command("DEL list")

    def test_existed_list(self):
        self.client.execute_command("RPUSH list value_a")
        self.client.execute_command("LPUSH list value_b")

        self.assertIntegerEqual("LLEN list", 2)

    def test_unexisted_list(self):
        self.assertIntegerEqual("LLEN list", 0)

    def test_not_list(self):
        self.client.execute_command("SET list value")

        self.assertSimpleErrorEqual("LLEN list", invalid_type("LLEN"))

    def test_wrong_arguments(self):
        self.assertSimpleErrorEqual("LLEN", wrong_argument("LLEN"))
        self.assertSimpleErrorEqual("LLEN list_a list_b", wrong_argument("LLEN"))

    def tearDown(self):
        self.client.execute_command("DEL list")
