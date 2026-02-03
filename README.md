
# ECS 34 Project 2 — DSV / XML Reader & Writer

This project implements a set of classes that is crucial for navigation and transportation planning:

- DSV：`CDSVReader` / `CDSVWriter`
- XML：`CXMLReader` / `CXMLWriter`

## Folder Structure

- `include/`: public headers
- `src/`: implementation
- `testsrc/`: GoogleTest unit tests
- `docs/`: API / usage docs

## Components

### DataSource and DataSink

- `CDataSource`: abstract input stream (`End/Get/Peek/Read`)
- `CDataSink`: abstract output stream (`Put/Write`)

### DSV

- `CDSVReader`: reads DSV from `CDataSource` and parses it into `std::vector<std::string>`
- `CDSVWriter`: writes `std::vector<std::string>` to `CDataSink` using DSV rules

### XML

- `CXMLReader`: streaming XML parser from `CDataSource`, outputs `SXMLEntity`
- `CXMLWriter`: writes `SXMLEntity` events as XML to `CDataSink`

### AI Usage

We use Claude Code to translate between English and Chinese in docs. Prompts like this: Translate the doc under docs folder and README.md from English text to Chinese. @./docs @README.md
