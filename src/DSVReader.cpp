#include "DSVReader.h"
#include "DataSource.h"
#include <memory>
#include <string>

struct CDSVReader::SImplementation {
    std::shared_ptr<CDataSource> DSource;
    char DDelimiter;

    SImplementation(std::shared_ptr<CDataSource> source, char delimiter) {
        DSource = source;
        DDelimiter = delimiter;
    }

    // reads one cell from the current row
    // returns true if it had read to the end of the line or end of file
    // otherwise, returns false
    bool ParseValue(std::string &value) {
        bool in_quotes = false;

        while (!DSource->End()) {
            char ch;
            // stores and consumes the current character at the head
            DSource->Get(ch);

            // flip the flags if we are currently not inside a quote
            if (ch == '"' && !in_quotes) {
                in_quotes = true;
                continue;
            }

            // if we are currently inside a quote, check what this quote is supposed to be
            if (ch == '"' && in_quotes) {
                char next_ch;
                // peek the next character
                DSource->Peek(next_ch);

                // if the next character is a delimiter, this quote is an End of Cell
                // but not End of Line, so we consume the delimiter and returns false
                if (next_ch == DDelimiter) {
                    DSource->Get(next_ch);
                    return false;
                }

                // if the next character is a newline or end of file,
                // this quote is also an End of Cell, but now we returns true instead
                if (next_ch == '\n' || DSource->End()) {
                    DSource->Get(next_ch);
                    return true;
                }

                // if the next character is another double quote, it's an escaped double quote
                // we consumes it and store it into ch, so that later we add it to our result string
                if (next_ch == '"') {
                    DSource->Get(ch);
                }

                // we decided to be permissive and not bail out when the next_ch is anything else,
                // and allow a singular double-quote inside a quoted cell to also be treated as a literal quote
                // this is just a design choice.
            }

            // if we are not in quotes and if we encountered the delimiter character,
            // it is an end of cell, but not end of row / end of line
            // returns false
            if (ch == DDelimiter && !in_quotes) {
                return false;
            }

            // if we are not in quotes and if we encountered a newline,
            // it is an end of cell and end of row / end of line
            // returns true
            if (ch == '\n' && !in_quotes) {
                return true;
            }

            value += ch;
        }

        // if we read to the end of source, we also returns true
        return true;
    }

    bool ReadRow(std::vector<std::string> &row) {
        std::string current_val;
        while (!ParseValue(current_val)) {
            row.push_back(current_val);
            current_val.clear();
        }
        row.push_back(current_val);

        return true;
    }

    // ParseValue and ReadRow should not bail out, so we just detect whether we consumed all sources
    bool End() { return DSource->End(); }
};

// pimpl
CDSVReader::CDSVReader(std::shared_ptr<CDataSource> source, char delimiter) {
    DImplementation = std::make_unique<SImplementation>(source, delimiter == '"' ? ',' : delimiter);
}

CDSVReader::~CDSVReader() = default;

bool CDSVReader::ReadRow(std::vector<std::string> &row) {
    return DImplementation->ReadRow(row);
}

bool CDSVReader::End() const { return DImplementation->End(); }
