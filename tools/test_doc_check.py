import unittest

from tools.doc_check import find_unlabelled_identifiers


class UnlabelledIdentifierTests(unittest.TestCase):
    def test_stack_offset_identifiers_accept_numeric_and_uppercase_hex_starts(self):
        source = "int sp8; int spB8; int spC;"

        self.assertEqual(find_unlabelled_identifiers(source), ["sp8", "spB8", "spC"])

    def test_ordinary_sp_prefixed_identifier_is_not_reported(self):
        self.assertEqual(find_unlabelled_identifiers("float speed;"), [])


if __name__ == "__main__":
    unittest.main()
