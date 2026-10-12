#include "BpString.h"

UINT64 BpRtlStringLength(
    _In_ CHAR16 *String
)
/*++
    Routine Description:
        Returns the length of a wide-character string.

    Arguments:

        String:
            Pointer to a null-terminated wide-character string.

    Return Value:
        The number of characters in the string, not including the null terminator.
--*/
{
    UINT64 Length = 0;

    while (String[Length] != L'\0')
        Length++;

    return Length;
}

CHAR16 BpRtlCharToUpper(
    _In_ CHAR16 Character
)
/*++
    Routine Description:
        Converts a wide character to uppercase.

    Arguments:

        Character:
            The wide character to convert.

    Return Value:
        The uppercase equivalent of the character, or the original character if not lowercase.
--*/
{
    if (Character >= L'a' && Character <= L'z')
        return (CHAR16)(Character - (L'a' - L'A'));

    return Character;
}

BOOLEAN BpRtlStringEqualsInsensitive(
    _In_ CHAR16 *Left,
    _In_ CHAR16 *Right
)
/*++
    Routine Description:
        Performs a case-insensitive comparison of two wide-character strings.

    Arguments:

        Left:
            Pointer to the first string to compare.

        Right:
            Pointer to the second string to compare.

    Return Value:
        TRUE if the strings are equal (case-insensitive), FALSE otherwise.
--*/
{
    while (*Left != L'\0' && *Right != L'\0')
    {
        if (BpRtlCharToUpper(*Left) != BpRtlCharToUpper(*Right))
            return FALSE;

        Left++;
        Right++;
    }

    return *Left == *Right;
}