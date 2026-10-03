"""Lift the actual object-special guards into focused native harnesses."""

from _paths import extract_function


def object_special_functions(*, dispatch=False, links=False):
    functions = []
    if links:
        functions.append(extract_function("affects.c", "P_char get_linked_char("))
    functions.append(extract_function("objmisc.c", "bool item_restricted_for_player_pet("))
    if dispatch:
        functions.append(extract_function("objmisc.c", "int invoke_object_special("))
    return "\n".join(functions)
