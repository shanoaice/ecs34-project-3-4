
# CDSVWriter

## Purpose

`CDSVWriter` writes one row (`std::vector<std::string>`) to `CDataSink`. It adds quotes and escapes when needed, based on DSV rules.

## Header / Dependencies

- Header: `include/DSVWriter.h`
- Dependency: `include/DataSink.h`

The constructor takes a `std::shared_ptr<CDataSink>`. The writer keeps this shared_ptr.

## Constructor

```cpp
CDSVWriter(std::shared_ptr<CDataSink> sink, char delimiter,
					 bool quoteall = false);
```

### Parameters

- `sink`
	- Output sink.

- `delimiter`
	- Field delimiter, like `','`, `'\t'`, `'|'`, etc.
	- Note: in the current implementation, if `delimiter == '"'`, it will be changed to `','`.

- `quoteall`
	- Whether to quote all fields.
	- `false`: only quote when the field has “special characters”.
	- `true`: always output as `"..."`.

## Main Methods

### `bool WriteRow(const std::vector<std::string> &row);`

Writes one row of fields to `sink`.

- Parameter
	- `row`: fields to write.

- Output rules
	- Write `delimiter` between fields.
	- Always write a trailing `\n` at the end of the row.
	- If any of the following is true, the field is wrapped in double quotes:
		- The field contains the delimiter `delimiter`
		- The field contains a newline `\n`
		- The field contains a double quote `"`
		- `quoteall == true`
	- If the field contains `"`, each `"` is replaced by `""`.

- Return
	- `true`.

## Usage

### Write CSV

```cpp
#include "DSVWriter.h"
#include "StringDataSink.h"

#include <memory>
#include <string>
#include <vector>

auto sink = std::make_shared<CStringDataSink>();
CDSVWriter writer(sink, ',');

writer.WriteRow({"Hello", "World"});
writer.WriteRow({"Has,Comma", "Has\"Quote\""});

// sink->String() looks like:
// Hello,World\n
// "Has,Comma","Has""Quote"""\n
```
