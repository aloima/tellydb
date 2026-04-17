import random
import sys
from pathlib import Path
from string import ascii_letters, digits

from tellypy import Client

utils_path = Path(__file__).resolve().parent.parent
sys.path.append(str(utils_path))

try:
    from utils import ExtendedTestCase, wrong_argument
except ImportError:
    sys.exit(1)


class EchoCommand(ExtendedTestCase):
    client: Client

    @classmethod
    def setUpClass(cls):
        cls.client = Client(host="localhost", port=6379)
        cls.client.connect()

    def test_without_arguments(self):
        self.assertSimpleErrorEqual("ECHO", wrong_argument("ECHO"))

    def test_with_argument(self):
        value = "".join(random.choices(ascii_letters + digits, k=64))
        self.assertBulkStringEqual(f"ECHO {value}", value)

    def test_with_arguments(self):
        self.assertSimpleErrorEqual("ECHO a b", wrong_argument("ECHO"))
