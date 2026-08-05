import unittest

from tools.doc_check import FUNC_DEF, find_documented_functions, find_unlabelled_identifiers


class UnlabelledIdentifierTests(unittest.TestCase):
    def test_stack_offset_identifiers_accept_numeric_and_uppercase_hex_starts(self):
        source = "int sp8; int spB8; int spC;"

        self.assertEqual(find_unlabelled_identifiers(source), ["sp8", "spB8", "spC"])

    def test_ordinary_sp_prefixed_identifier_is_not_reported(self):
        self.assertEqual(find_unlabelled_identifiers("float speed;"), [])

    def test_line_and_block_comments_are_ignored(self):
        source = '''
        // arg1 "unterminated quote does not escape the comment
        /* temp_r2 "quoted text" spB8 */
        int var_r4;
        '''

        self.assertEqual(find_unlabelled_identifiers(source), ["var_r4"])

    def test_strings_with_comment_markers_and_character_literals_are_ignored(self):
        source = r'''
        const char *line = "arg2 // temp_r3";
        const char *block = "var_r4 /* spB8 */";
        char quote = '\'';
        char slash = '/';
        '''

        self.assertEqual(find_unlabelled_identifiers(source), [])

    def test_compact_member_accesses_are_ignored(self):
        source = "object.arg2 = pointer->temp_r3 + var_r4;"

        self.assertEqual(find_unlabelled_identifiers(source), ["var_r4"])

    def test_spaced_and_multiline_member_accesses_are_ignored(self):
        source = '''
        s32 arg1;
        object
            .
            arg0 = pointer ->
            temp_r2 + var_r3;
        '''

        self.assertEqual(find_unlabelled_identifiers(source), ["arg1", "var_r3"])

    def test_callback_parameter_is_scanned_without_hiding_outer_definition(self):
        source = "void CallbackSet(void (*callback)(s16 arg0)) {}"
        matches = list(FUNC_DEF.finditer(source))

        self.assertEqual([match.group(1) for match in matches], ["CallbackSet"])
        self.assertEqual(find_unlabelled_identifiers(source), ["arg0"])


class FunctionDefinitionTests(unittest.TestCase):
    def test_weak_and_multiline_definitions_are_detected_and_documented(self):
        source = '''
        /** Weak definition. */
        __declspec(weak) void WeakDefinition(void) {
        }

        /** Multiline definition. */
        static s32
        MultilineDefinition(
            s32 value
        ) {
            return value;
        }
        '''
        matches = list(FUNC_DEF.finditer(source))

        self.assertEqual(
            [match.group(1) for match in matches],
            ["WeakDefinition", "MultilineDefinition"],
        )
        self.assertEqual(
            find_documented_functions(source, matches),
            {"WeakDefinition", "MultilineDefinition"},
        )

    def test_compound_literal_call_is_not_a_function_definition(self):
        source = '''
        void Setup(void) {
            ApplyColor(&(GXColor){0, 0, 0, 0});
        }
        '''

        self.assertEqual(FUNC_DEF.findall(source), ["Setup"])


if __name__ == "__main__":
    unittest.main()
