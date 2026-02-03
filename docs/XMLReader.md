
# CXMLReader

## Purpose

`CXMLReader` parses XML from `CDataSource` in a streaming way, and outputs `SXMLEntity` events one by one (start tag / end tag / character data).

## Header / Dependencies

- Header: `include/XMLReader.h`
- Dependencies:
	- `include/DataSource.h`
	- `include/XMLEntity.h`

It uses the `Expat` parser.

## Constructor

```cpp
CXMLReader(std::shared_ptr<CDataSource> src);
```

### Parameters

- `src`
	- Input data source.
	- The reader reads input in blocks and feeds it into the XML parser.

## Main Methods

### `bool End() const;`

Checks whether the XML document has been fully read.

- Return
	- `true`: already at end of the XML document.
	- `false`: parsing is not finished.

### `bool ReadEntity(SXMLEntity &entity, bool skipcdata = false);`

Reads the next XML entity event.

- Parameters
	- `entity`: output parameter, receives the next entity on success.
	- `skipcdata`: whether to skip character data.
		- `true`: before returning the next entity, it drops all queued CharData events.
		- `false`: CharData events are returned in parse order.

- Entity types and fields
	- `StartElement`
		- `DNameData`: tag name
		- `DAttributes`: attribute list
	- `EndElement`
		- `DNameData`: tag name
	- `CharData`
		- `DNameData`: character data
		- Note: the current implementation drops CharData that is only whitespace (for example, pure whitespace from newlines/indentation).

- Return
	- `true`: successfully read one entity and wrote it into `entity`.
	- `false`: reached end of document, or parse failed.