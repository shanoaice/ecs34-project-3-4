#include "DSVWriter.h"
#include "DataSink.h"
#include "StringUtils.h"
#include <memory>
#include <string>
#include <vector>

struct CDSVWriter::SImplementation {
    std::shared_ptr<CDataSink> DSink;
    char DDelimiter;
    bool DQuoteAll;

    SImplementation(std::shared_ptr<CDataSink> sink, char delim,
                    bool quoteall) {
        DSink = sink;
        DDelimiter = delim;
        DQuoteAll = quoteall;
    }

    bool WriteRow(const std::vector<std::string> &row) {
        bool first_col = true;
        for (const auto &col : row) {
            if (!first_col) {
                DSink->Put(DDelimiter);
            }
            first_col = false;

            std::vector<char> buf;

            // if there are quotes, escape by doubling them
            if (col.find("\"") != std::string::npos) {
                auto escaped = StringUtils::Replace(col, "\"", "\"\"");
                buf.assign(escaped.cbegin(), escaped.cend());
            } else {
                buf.assign(col.cbegin(), col.cend());
            }

            // if there are special characters, quote the string
            // or if quoteall is enabled
            if (col.find(DDelimiter) != std::string::npos ||
                col.find("\n") != std::string::npos ||
                col.find("\"") != std::string::npos || DQuoteAll) {
                DSink->Put('"');
                DSink->Write(buf);
                DSink->Put('"');
            // otherwise, write the original buf
            } else {
                DSink->Write(buf);
            }
        }

        DSink->Put('\n');

        return true;
    };
};

CDSVWriter::CDSVWriter(std::shared_ptr<CDataSink> sink, char delimiter,
                       bool quoteall) {
    DImplementation = std::make_unique<SImplementation>(
        sink, delimiter == '"' ? ',' : delimiter, quoteall);
}

CDSVWriter::~CDSVWriter() = default;

bool CDSVWriter::WriteRow(const std::vector<std::string> &row) {
    return DImplementation->WriteRow(row);
}
