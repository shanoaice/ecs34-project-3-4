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

    bool ParseValue(std::string &value) {
        // if read until the end of the row.

        bool in_quotes = false;

        while (!DSource->End()) {
            char ch;
            DSource->Get(ch);

            if (ch == '"' && !in_quotes) {
                in_quotes = true;
                continue;
            }

            if (ch == '"' && in_quotes) {
                char next_ch;
                DSource->Peek(next_ch);

                if (next_ch == DDelimiter) {
                    DSource->Get(next_ch);
                    return false;
                }

                if (next_ch == '\n' || DSource->End()) {
                    DSource->Get(next_ch);
                    return true;
                }

                if (next_ch == '"') {
                    DSource->Get(ch);
                }
            }

            if (ch == DDelimiter && !in_quotes) {
                return false;
            }

            if (ch == '\n' && !in_quotes) {
                return true;
            }

            value += ch;
        }

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

    bool End() { return DSource->End(); }
};

CDSVReader::CDSVReader(std::shared_ptr<CDataSource> source, char delimiter) {
    DImplementation = std::make_unique<SImplementation>(source, delimiter);
}

CDSVReader::~CDSVReader() = default;

bool CDSVReader::ReadRow(std::vector<std::string> &row) {
    return DImplementation->ReadRow(row);
}

bool CDSVReader::End() const { return DImplementation->End(); }
