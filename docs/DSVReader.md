# CDSVReader

## Purpose

`CDSVReader` reads DSV text from `CDataSource` line by line, and parses one line into a list of cells (`std::vector<std::string>`).

## Header / Dependencies

- Header: `include/DSVReader.h`
- Dependency: `include/DataSource.h`

The constructor takes a `std::shared_ptr<CDataSource>`. The reader keeps this shared_ptr and reads the DSV file from it.

## Constructor

```cpp
CDSVReader(std::shared_ptr<CDataSource> src, char delimiter);
```

### Parameters

- `src`
	- Input data source.
	- The reader reads the DSV file from the `CDataSource`.

- `delimiter`
	- Field delimiter, like `','`, `'\t'`, `'|'`, etc.
	- Note: the quote character `"` treated as a `,` comma.

## Main Methods

### `bool ReadRow(std::vector<std::string> &row);`

Reads and parses the fields of the “next line”.

- Parameter
	- `row`: output parameter, receives parsed fields.

- Parse rules
	- **A field ends with `delimiter` by default.**
	- **A row ends with `\n` by default.**
	- When the reader sees `"` and it is not currently in quote mode, it enters **quote mode**.
		- In quote mode, `delimiter` and `\n` are treated as normal field characters.
		- `""` is parsed as one `"`.
		- After the closing quote:
			- If the next char is `delimiter`: the field ends and the next field begins.
			- If the next char is `\n` or end-of-file: the row ends.

- Return
	- `true` if read successfully.
	- `false` if it fails.
