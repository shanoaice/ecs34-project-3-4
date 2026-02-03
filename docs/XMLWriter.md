
# CXMLWriter

## Purpose

`CXMLWriter` serializes `SXMLEntity` events into XML text and writes to `CDataSink`. It keeps a stack of open tags to make sure the output is well-formed.

## Header / Dependencies

- Header: `include/XMLWriter.h`
- Dependencies:
	- `include/DataSink.h`
	- `include/XMLEntity.h`

## Constructor

```cpp
CXMLWriter(std::shared_ptr<CDataSink> sink);
```

### Parameters

- `sink`
	- Output sink.
	- The writer outputs text via `Write(const std::vector<char>&)`.

## Main Methods

### `bool WriteEntity(const SXMLEntity &entity);`

Writes one XML entity event to the output.

- Parameter
	- `entity`: the entity to write.


- Supported entity types and output behavior
	- `SXMLEntity::EType::StartElement`
		- Requires: `entity.DNameData` is not empty.
		- Output: `<name attr="value">`

	- `SXMLEntity::EType::EndElement`
		- Requires: `entity.DNameData` is not empty, and it must exactly match the name at the top of the “open tag stack”.
		- Output: `</name>`
		- If it does not match: the function returns `false`.

	- `SXMLEntity::EType::CharData`
		- Output: writes character data text, with escaping.

	- `SXMLEntity::EType::CompleteElement`
		- Requires: `entity.DNameData` is not empty.
		- Output: `<name attr="value"/>`

- Attribute rules
	- Attributes are written in the current order of `entity.DAttributes`.
	- Each attribute is written as ` key="value"`.

- Escape rules
	- Text (CharData) escapes:
		- `&` → `&amp;`
		- `<` → `&lt;`
		- `>` → `&gt;`
	- Attribute values escape:
		- `&` / `<` / `>` same as above
		- `"` → `&quot;`
		- `\\` → `&apos;`

- Return
	- `true`: write succeeded.
	- `false`: the end tag does not match the stack.

### `bool Flush();`

Closes all currently unclosed start tags.

- Return
	- `true`: all tags were closed.
	- `false`: underlying sink write failed.

## Usage

### Write an XML

```cpp
#include "XMLWriter.h"
#include "StringDataSink.h"

#include <memory>

auto sink = std::make_shared<CStringDataSink>();
CXMLWriter writer(sink);

SXMLEntity rootBegin{SXMLEntity::EType::StartElement, "root", {}};
SXMLEntity text{SXMLEntity::EType::CharData, "hello", {}};
SXMLEntity rootEnd{SXMLEntity::EType::EndElement, "root", {}};

writer.WriteEntity(rootBegin);
writer.WriteEntity(text);
writer.WriteEntity(rootEnd);

// Output: <root>hello</root>
```

### Use CompleteElement for a self-closing tag

```cpp
SXMLEntity empty{SXMLEntity::EType::CompleteElement, "empty_element", {}};
writer.WriteEntity(empty);
// Output: <empty_element/>
```

