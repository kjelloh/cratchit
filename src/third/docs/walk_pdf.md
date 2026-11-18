# walk_pdf

> In cratchit, walk_pdf (session 9fe36d28) runs from the command line as
> `cratchit --walk-pdf ...`, without cratchit's user interface. `cratchit
> --walk-pdf --help` prints this manual. cratchit's own user manual is
> separate.

walk_pdf walks the structure of a PDF file and shows each part it visits as a
*path* (where in the file it is) and *content* (what is there). The main aim is
the text of the PDF, so that it can be turned into plain text or Markdown.

> Status: early. walk_pdf finds the file's revisions, their top-level
> elements, and what is inside objects and trailers: dictionary entries, array
> items and stream data. It decrypts files that open without a password,
> decompresses streams, splits page content into text blocks and their
> operators, follows references, e.g. from a text block to its font, and shows
> the text each text block holds. It walks pages in display order, and
> writes a document's text as plain text (`--text`).

## Usage

```
cratchit --walk-pdf <file.pdf> [<path>] [--depth N]
cratchit --walk-pdf --text <file.pdf>
cratchit --walk-pdf [options]
```

walk_pdf lists the element at `path` and what is inside it, `N` levels down,
depth first: each element is followed by its children before its next
sibling. Without a path it starts at `/`, the whole file. Without `--depth` it
goes 3 levels down.

`cratchit --walk-pdf <file.pdf>` prints one line per element:

```
    offset       size  path                             peek
         0     489247  /                                1 revision
         0     489246  /rev[0]                          53 elements: 48 objects, xref, trailer
         0          8  /rev[0]/header                   version 1.4
         9        167  /rev[0]/obj[1]                   << /Title "Invoice" /Creator "wkhtmltopdf 0.12.4" /Produc...
        27         18  /rev[0]/obj[1]/Title             "Invoice"
        55         40  /rev[0]/obj[1]/Creator           "wkhtmltopdf 0.12.4"
       ...
     55726        120  /rev[0]/obj[5]                   << /Type /Page /Parent 2 0 R /Contents 35 0 R /Resources ...
     55743          5  /rev[0]/obj[5]/Type              /Page
     55757          5  /rev[0]/obj[5]/Parent            2 0 R
       ...
     56069     423950  /rev[0]/obj[35]                  << /Length 36 0 R /F... + stream <423875 bytes, FlateDecode>
     56089          6  /rev[0]/obj[35]/Length           36 0 R
     56104         12  /rev[0]/obj[35]/Filter           /FlateDecode
     56127     423875  /rev[0]/obj[35]/stream           <423875 bytes>
```

(The heading line is shown here for explanation only; walk_pdf does not print it.)

- *offset* is the element's byte offset in the file, and *size* its length in bytes.
  Inside a decoded stream, which is not in the file, the offset is in the
  decoded data and starts with `+`, e.g. `+0`.
- *path* says which element it is (see Paths below).
- *peek* is a short look at what the element holds (see Peek below).

`cratchit --walk-pdf <file.pdf> <path>` starts at the element at `path`. Quote the path in
the shell, as `[` and `]` mean something to most shells:

```
cratchit --walk-pdf book.pdf '/rev[1]/trailer[0]'
cratchit --walk-pdf book.pdf '/rev[1]' --depth 0     # only the revision itself
```

## Peek

The peek column shows what an element holds, without the keyword that opens it
(`12 0 obj`, `trailer`, ...), since the path already says what it is:

| Element    | Peek                                                              |
|------------|-------------------------------------------------------------------|
| `/`        | How many revisions and pages the file has, and if it is encrypted, how and which permissions it does not grant (see Encrypted files below) |
| `/pages`   | How many pages there are |
| page       | Its page object: `-> /rev[0]/obj[5]` |
| revision   | How many elements it has, e.g. `19 elements: 13 objects, xref, trailer` |
| header     | The PDF version, e.g. `version 1.3`                               |
| comment    | Its text, or `<binary marker, n bytes>` for the line that tells tools the file holds binary data |
| object     | Its body, e.g. `<< /Type /Page ... >>`. If it holds a stream, its dictionary followed by `+ stream <n bytes, filters>`, where the filters (e.g. `FlateDecode`) say how the data is compressed or encoded |
| xref       | How many entries it has, and for which object numbers             |
| trailer    | Its dictionary                                                    |
| startxref  | The byte offset of the cross-reference                            |
| eof        | Nothing                                                           |
| unknown    | Its text, or `<n bytes, binary>`                                  |
| a value inside an object: dictionary, array, name, number, boolean, null | Its text |
| reference  | The reference and where it leads: `37 0 R -> /rev[0]/obj[37]`, or `-> not found` |
| string     | Text strings marked as UTF-16 or UTF-8 are shown decoded, in quotes: `"Invoice"`. Other strings as they are in the file: `(D:20261012)` |
| stream     | The stream's data if it is text (an uncompressed page, say), otherwise `<n bytes, filters>`, e.g. `<423875 bytes, FlateDecode>` |
| text block | The text it shows, in quotes: `"Inköp av råvaror och material i Sverige,"` (see Text below). If its font is not found, the block itself: `BT /F8 16 Tf ... ET` |
| operation  | Its operands and operator, e.g. `1 0 0 -1 0 0 Tm`, or `<inline image, n bytes>`. A text-showing operator (`Tj`, `TJ`, `'`, `"`) shows its text: `Tj "H"`. A font selection says which font: `/F8 16 Tf -> /rev[0]/obj[8]` |
| decoded    | The stream's decoded data if it is text (e.g. a page's drawing instructions), otherwise `<n bytes>`. If decoding stopped early, why, first: `(FlateDecode: data ends early, ...)` |

Runs of whitespace are shown as one space, and other bytes outside printable
ASCII as `.`. Text that is too long is cut and ends with `...`. A peek is at
most 60 characters, except that a stream's description is always shown in
full, so an object with a stream can be a little wider.

Dictionaries and arrays are shown rebuilt from their entries and items, so that
the strings in them are decoded too: `<< /Title "Invoice" /Creator "wkhtmltopdf 0.12.4" ... >>`.

A PDF text string is decoded when it starts with a mark saying how it is
encoded: `FE FF` for UTF-16 (big-endian) and `EF BB BF` for UTF-8. Bytes that
do not decode are shown as `�` (U+FFFD). Strings without a mark use
PDFDocEncoding, which is not decoded yet.

A stream's data in the file is usually compressed, and in an encrypted file
also encrypted. Its `decoded` child is the data decrypted and with its filters
applied: `FlateDecode` (zlib, with PNG and TIFF predictors), `ASCIIHexDecode`
and `ASCII85Decode`. Streams with other filters, such as images (`DCTDecode`),
have no `decoded` child.

## Encrypted files

A PDF can be encrypted (its trailer has `/Encrypt`). Most such files open
without a password: the key is computed from values in the file itself, which
is how a browser shows them. walk_pdf does the same, for the Standard security
handler with RC4 (40 to 128 bits). It then shows strings and stream data
decrypted in the peek column. Strings that are not marked as UTF-16 or UTF-8
are shown decrypted in parentheses: `(Bilagor)`.

The `/` row says how the file is encrypted, and which permissions its owner
does not grant, e.g.

```
2 revisions; encrypted: RC4 128-bit, no password needed; not permitted: modify, copy/extract, annotate, extract for accessibility
```

The peek is a look at the structure, cut to one line per element. It is not a
way to extract a file's text: walk_pdf will honour "copy/extract" when it gets
a text output.

Not decrypted: files that need a password, and files encrypted with AES
(which shows as `AES-128: not supported yet` or `AES-256: not supported yet`).
Their strings and streams are shown as they are in the file.

## References

An object refers to another with a reference: `37 0 R` is object 37. walk_pdf
follows it to the latest revision that has object 37, and shows where it leads
in the peek: `37 0 R -> /rev[0]/obj[37]`. Give that path to walk_pdf to look
at the object.

A text block selects a font with `Tf`, by a name such as `/F8`. The name is
looked up in the resources the content stream draws with: its page's
`/Resources` (or, if the page has none, the nearest one up its page tree
through `/Parent`), or for a form its own `/Resources`. Then `/Font/F8` there
leads to the font, shown in the peek: `/F8 16 Tf -> /rev[0]/obj[8]`.

Objects stored inside object streams (`/Type /ObjStm`, PDF 1.5 and later) are
found too (cratchit's addition to walk_pdf): a reference to one leads into the
stream's decoded data, e.g. `3 0 R -> /rev[0]/obj[360]/stream/decoded/obj[3]`, and
the decoded data of an object stream lists its objects as children.

## Text

The bytes of a text string in a page are *character codes* of the string's
font, not text in any standard encoding. walk_pdf turns them into text the way
the font says:

1. It finds the font: the last `Tf` before the string in its content stream
   (with `q` and `Q` saving and restoring it), and the font that name stands
   for in the page's resources (see References).
2. It splits the bytes into codes: two bytes each for a composite (`Type0`)
   font with `/Identity-H` or `/Identity-V`, as an embedded CMap says, or one
   byte each for a simple font (`Type1`, `TrueType`, `Type3`).
3. It maps each code to Unicode by the font's `/ToUnicode` map, or for a
   simple font without one, by its `/Encoding`: `WinAnsiEncoding`,
   `MacRomanEncoding` or `StandardEncoding`, changed by `/Differences`. Glyph
   names in `/Differences` are known for Latin text (`adieresis` is ä) and as
   `uniXXXX`. A code that maps to nothing is shown as `�`.

A text block's text is its strings in order. A new line (`T*`, `'`, `"`, a
`Tm`, or a `Td`/`TD` that moves down) and a large gap in a `TJ` array (a
quarter of the font size or more) become a space. walk_pdf does not yet look
at where on the page text is placed, so the order is that of the content stream.

Text drawn as shapes, rather than with a font, has no characters, and is not
found. Some PDF makers draw all text that way.

## Display order

Everything above follows the file's layout: revisions, objects, streams, in
the order the bytes are in the file. Pages are *shown* in another order: the
catalog's page tree, and in each page its content streams and the forms they
draw. walk_pdf walks that order under `/pages`:

```
cratchit --walk-pdf file.pdf /pages --depth 1           the pages, in order
cratchit --walk-pdf file.pdf '/pages/page[0]' --depth 1  the first page's drawing operations
```

| Path                         | Element                                              |
|------------------------------|------------------------------------------------------|
| `/pages`                     | All pages, in page tree order                        |
| `/pages/page[i]`             | Page i, counted from 0                               |
| `/pages/page[i]/op[j]`       | Operation j of the page: all its content streams' operations, in order |
| `.../op[j]/[k]`              | Operand k of an operation                            |
| `.../op[j]/op[k]`            | When op[j] draws a form (`/Fm1 Do`), operation k of the form |

`/pages` is not listed under `/`, so a plain `cratchit --walk-pdf file.pdf` does not list
every drawing operation; the `/` row says how many pages there are.

Text-showing operations show their text, as elsewhere. The font in effect is
followed through the page in drawing order, across its content streams, with
`q`/`Q`, into the forms it draws. A form drawn with `Do` uses its own
`/Resources` (or else its page's); forms drawing forms are followed eight
levels deep at most, so a form that draws itself does not go on forever.

Pages and fonts kept in object streams (`/Type /ObjStm`) are found as any other
object.

## Plain text

`cratchit --walk-pdf --text file.pdf` prints the document's text to standard output,
each page after a `--- page n ---` line. walk_pdf writes no files; to keep the
text, redirect it: `cratchit --walk-pdf --text file.pdf > file.txt`. Errors go to
standard error, and so do warnings: when no pages were found, or when some characters could not be mapped (shown as `�`).

cratchit gets the same text through walk_pdf's API, `extract_text`; see
`src/walk_pdf/walk_pdf.hpp`.

For example:

```
--- page 1 ---
Invoice

Invoice number AXLBEPCT-0003

Date of issue August 30, 2025
```

walk_pdf goes through each page in display order, as under `/pages`, and
follows where each piece of text is drawn: the transformations (`cm`, a form's
`/Matrix`), the text position (`Tm`, `Td`, `TD`, `T*`, `'`, `"`), and how far
each glyph moves it (the font's widths, the size, `Tc`, `Tw`, `Tz`). Then:

- Text on the same baseline (within half a font size) is one line, in order
  along it. Text turned a quarter, as on a page printed sideways, reads along
  its own lines.
- Two pieces of a line are joined with a space when the gap between them is
  wider than a fifth of the font size, and without one otherwise. A word drawn
  in pieces is whole again.
- Lines go top to bottom, with an empty line where the gap to the next is more
  than one and a half lines.
- Marked content with `/ActualText` (`/Span <</ActualText (-)>> BDC ... EMC`)
  gives that text in place of the text its glyphs map to.

Text whose font is not found is left out. Two columns side by side come out as
one line each, the columns' lines joined.

An encrypted file whose permissions do not allow copying or extracting its
text (`/P`, see Encrypted files) is refused, with exit code 1. walk_pdf still
shows its structure.

## Paths

A path names one element of the file, from the top down, like a file system
path. It is plain text: the same path names the same element every time
walk_pdf is run on the same file. `/` is the whole file.

A PDF file is its original part followed by any updates appended to it later.
Each of these is a *revision*, and runs up to and including its `%%EOF` line.
An update may store a new version of an object under the same object number,
so the revision is part of the path: `/rev[0]/obj[5]` and `/rev[1]/obj[5]`
are two versions of object 5.

| Path segment     | Element                                              |
|------------------|------------------------------------------------------|
| `rev[i]`         | Revision i of the file, counted from 0               |
| `header`         | `%PDF-x.y`, the version line at the start of the file |
| `comment[i]`     | A comment line, e.g. the line of binary marker bytes |
| `obj[n]`         | Indirect object number n: `n g obj ... endobj`       |
| `xref[i]`        | A classic cross-reference table                      |
| `trailer[i]`     | `trailer << ... >>`                                  |
| `startxref[i]`   | `startxref` and the offset of its cross-reference    |
| `eof[i]`         | `%%EOF`                                              |
| `unknown[i]`     | Bytes walk_pdf could not recognise                   |
| *key*            | The value of a dictionary entry, e.g. `Type`, `Resources` |
| `[i]`            | Item i of an array, counted from 0                   |
| `stream`         | The data of a stream object, as it is in the file    |
| `decoded`        | A stream's data decrypted and decompressed (not in the file) |
| `BT[i]`          | Text block i (`BT ... ET`) of a content stream, counted from 0 |
| `op[i]`          | Operation i of a text block: operands and an operator, e.g. `/F8 16 Tf` |

What is inside what:

- A revision holds its header, comments, objects, xref, trailer, startxref and eof.
- An object holding a dictionary has the dictionary's entries as children,
  followed by `stream` if it is a stream object. An object holding an array
  has its items. A trailer has its dictionary's entries.
- A dictionary value has its entries, and an array value its items. Other
  values have no children.
- A stream has `decoded` as its child, when walk_pdf can apply its filters.
- A decoded *content stream*, the drawing instructions of a page or form, has
  its text blocks `BT[i]` as children. A text block has its operations `op[i]`
  (without its `BT` and `ET`), and an operation has its operands `[i]`. Drawing
  instructions outside text blocks are not listed.

walk_pdf does not yet follow a page to its `/Contents`, so it tells content
streams from other streams by their dictionary and data: a stream is taken
as a content stream unless its `/Type` or `/Subtype` says otherwise (an image,
font, object stream and so on), it is a font program (`/Length1`), its data
is a CMap (`begincmap`, e.g. a font's `/ToUnicode`), or its data is binary.

For example, `/rev[0]/obj[35]/stream/decoded/BT[0]/op[3]` is the fourth
operation of the first text block of the page drawn by object 35, and
`.../op[3]/[0]` its first operand.

For example, `/rev[0]/obj[5]/Resources/Font/F8` is the `/F8` entry of the
`/Font` dictionary of the `/Resources` dictionary of object 5, and
`/rev[0]/obj[5]/MediaBox/[2]` is the third number of its `/MediaBox` array.

A key is written as it is, except that a byte which would clash with the path
syntax (whitespace, `/`, `[`, `]` and other PDF delimiters, `#`, or a byte
outside printable ASCII) is written `#` and two hex digits, as in PDF names:
the key `A B` is `A#20B`. Keys that are spelled like the reserved segments
`header`, `stream` and `decoded` get their first letter written that way: `#68eader`.
The empty key is `#`.

An object is named by its object number alone: `12 0 obj` is `obj[12]`. The
second number in the file, the generation, is left out: each revision has at
most one object with a given number, so the revision and the number are enough.
(The generation is 0 for nearly every object; it only goes up when an update
reuses the number of a deleted object.) Path strings contain no blanks.

Elements of a revision other than objects are numbered in file order from 0
within their revision, per kind.

Files made for fast web viewing ("linearized") have two revisions: a small
first part for the first page, then the rest.

PDF 1.5 and later may keep the cross-reference in a stream object
(`/Type /XRef`) instead of an `xref` table. That is an ordinary `obj[n]`, and
its revision has no `xref` or `trailer`.

## Options

| Option         | What it does                                                    |
|----------------|-----------------------------------------------------------------|
| `--text`       | Print the document's text as plain text, page by page, instead of its structure (see Plain text). |
| `--depth N`    | How many levels below the path to list. 0 lists only the element at the path, 1 adds its children, and so on. Default 3. |
| `-h`, `--help` | Print this manual and exit.                                     |

With the default depth of 3, walk_pdf lists revisions, their elements, and the
entries and items of objects and trailers, but not what is nested deeper.
`--depth 4` also lists each stream's decoded data, and `--depth 7` everything
down to the operands of text operators. To look into one page's text, start at
its decoded stream: `cratchit --walk-pdf file.pdf '/rev[0]/obj[35]/stream/decoded' --depth 2`. Give
a path and a depth to look further into one part, e.g.
`cratchit --walk-pdf file.pdf '/rev[0]/obj[5]' --depth 3`.

Running `cratchit --walk-pdf` without further arguments prints walk_pdf's name and version, and cratchit's.

## Exit codes

| Code | Meaning                                                   |
|------|-----------------------------------------------------------|
| 0    | Success.                                                  |
| 1    | The file could not be read, or has no element at the path. With `--text`: also not a PDF, encrypted in a way walk_pdf can not open, or its permissions do not allow copying its text. |
| 2    | Invalid command line arguments, or an invalid path.       |
