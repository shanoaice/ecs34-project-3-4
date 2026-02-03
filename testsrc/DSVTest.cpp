#include "DSVReader.h"
#include "DSVWriter.h"
#include "StringDataSink.h"
#include "StringDataSource.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

std::vector<std::string> row1 = {"Hello", "World"};
std::vector<std::string> row2 = {"Foo|WithDelim", "Bar,WithComma"};
std::vector<std::string> row3 = {"Baz\nWithNewline", R"(Qux"WithQuote)"};

TEST(DSVWriter, Comma) {
    auto Sink = std::make_shared<CStringDataSink>();
    CDSVWriter Writer(Sink, ',');

    Writer.WriteRow(row1);
    EXPECT_EQ(Sink->String(), "Hello,World\n");

    Writer.WriteRow(row2);
    EXPECT_EQ(Sink->String(), R"(Hello,World
Foo|WithDelim,"Bar,WithComma"
)");

    Writer.WriteRow(row3);
    EXPECT_EQ(Sink->String(), R"(Hello,World
Foo|WithDelim,"Bar,WithComma"
"Baz
WithNewline","Qux""WithQuote"
)");
}

TEST(DSVWriter, CommaQuoted) {
    auto SinkQuoted = std::make_shared<CStringDataSink>();
    CDSVWriter QuotedWriter(SinkQuoted, ',', true);
    QuotedWriter.WriteRow(row1);
    EXPECT_EQ(SinkQuoted->String(), R"("Hello","World"
)");

    QuotedWriter.WriteRow(row2);
    EXPECT_EQ(SinkQuoted->String(), R"("Hello","World"
"Foo|WithDelim","Bar,WithComma"
)");

    QuotedWriter.WriteRow(row3);
    EXPECT_EQ(SinkQuoted->String(), R"("Hello","World"
"Foo|WithDelim","Bar,WithComma"
"Baz
WithNewline","Qux""WithQuote"
)");
}

TEST(DSVWriter, Delim) {
    auto SinkDelim = std::make_shared<CStringDataSink>();
    CDSVWriter DelimWriter(SinkDelim, '|');
    DelimWriter.WriteRow(row1);
    EXPECT_EQ(SinkDelim->String(), "Hello|World\n");

    DelimWriter.WriteRow(row2);
    EXPECT_EQ(SinkDelim->String(), R"(Hello|World
"Foo|WithDelim"|Bar,WithComma
)");

    DelimWriter.WriteRow(row3);
    EXPECT_EQ(SinkDelim->String(), R"(Hello|World
"Foo|WithDelim"|Bar,WithComma
"Baz
WithNewline"|"Qux""WithQuote"
)");
}

TEST(DSVWriter, DelimQuoted) {
    auto SinkDelimQuoted = std::make_shared<CStringDataSink>();
    CDSVWriter DelimQuotedWriter(SinkDelimQuoted, '|', true);
    DelimQuotedWriter.WriteRow(row1);
    EXPECT_EQ(SinkDelimQuoted->String(), R"("Hello"|"World"
)");

    DelimQuotedWriter.WriteRow(row2);
    EXPECT_EQ(SinkDelimQuoted->String(), R"("Hello"|"World"
"Foo|WithDelim"|"Bar,WithComma"
)");

    DelimQuotedWriter.WriteRow(row3);
    EXPECT_EQ(SinkDelimQuoted->String(), R"("Hello"|"World"
"Foo|WithDelim"|"Bar,WithComma"
"Baz
WithNewline"|"Qux""WithQuote"
)");
}

TEST(DSVReader, Comma) {
    auto Sink = std::make_shared<CStringDataSink>();
    CDSVWriter Writer(Sink, ',');

    Writer.WriteRow(row1);
    Writer.WriteRow(row2);
    Writer.WriteRow(row3);

    auto Source = std::make_shared<CStringDataSource>(
        std::string(Sink->String().cbegin(),
                    Sink->String().cend() - 2)); // strip away the final newline
    CDSVReader Reader(Source, ',');

    std::vector<std::string> read_row1;
    std::vector<std::string> read_row2;
    std::vector<std::string> read_row3;

    Reader.ReadRow(read_row1);
    Reader.ReadRow(read_row2);
    Reader.ReadRow(read_row3);

    EXPECT_EQ(read_row1, row1);
    EXPECT_EQ(read_row2, row2);
    EXPECT_EQ(read_row3, row3);

    EXPECT_TRUE(Reader.End());
}

TEST(DSVReader, CommaQuoted) {
    auto SinkQuoted = std::make_shared<CStringDataSink>();
    CDSVWriter Writer(SinkQuoted, ',', true);

    Writer.WriteRow(row1);
    Writer.WriteRow(row2);
    Writer.WriteRow(row3);

    auto Source = std::make_shared<CStringDataSource>(std::string(
        SinkQuoted->String().cbegin(),
        SinkQuoted->String().cend() - 2)); // strip away the final newline
    CDSVReader Reader(Source, ',');

    std::vector<std::string> read_row1;
    std::vector<std::string> read_row2;
    std::vector<std::string> read_row3;

    Reader.ReadRow(read_row1);
    Reader.ReadRow(read_row2);
    Reader.ReadRow(read_row3);

    EXPECT_EQ(read_row1, row1);
    EXPECT_EQ(read_row2, row2);
    EXPECT_EQ(read_row3, row3);

    EXPECT_TRUE(Reader.End());
}

TEST(DSVReader, Delim) {
    auto SinkDelim = std::make_shared<CStringDataSink>();
    CDSVWriter Writer(SinkDelim, '|');

    Writer.WriteRow(row1);
    Writer.WriteRow(row2);
    Writer.WriteRow(row3);

    auto Source = std::make_shared<CStringDataSource>(std::string(
        SinkDelim->String().cbegin(),
        SinkDelim->String().cend() - 2)); // strip away the final newline
    CDSVReader Reader(Source, '|');

    std::vector<std::string> read_row1;
    std::vector<std::string> read_row2;
    std::vector<std::string> read_row3;

    Reader.ReadRow(read_row1);
    Reader.ReadRow(read_row2);
    Reader.ReadRow(read_row3);

    EXPECT_EQ(read_row1, row1);
    EXPECT_EQ(read_row2, row2);
    EXPECT_EQ(read_row3, row3);

    EXPECT_TRUE(Reader.End());
}

TEST(DSVReader, DelimQuoted) {
    auto SinkDelimQuoted = std::make_shared<CStringDataSink>();
    CDSVWriter Writer(SinkDelimQuoted, '|', true);

    Writer.WriteRow(row1);
    Writer.WriteRow(row2);
    Writer.WriteRow(row3);

    auto Source = std::make_shared<CStringDataSource>(std::string(
        SinkDelimQuoted->String().cbegin(),
        SinkDelimQuoted->String().cend() - 2)); // strip away the final newline
    CDSVReader Reader(Source, '|');

    std::vector<std::string> read_row1;
    std::vector<std::string> read_row2;
    std::vector<std::string> read_row3;

    Reader.ReadRow(read_row1);
    Reader.ReadRow(read_row2);
    Reader.ReadRow(read_row3);

    EXPECT_EQ(read_row1, row1);
    EXPECT_EQ(read_row2, row2);
    EXPECT_EQ(read_row3, row3);

    EXPECT_TRUE(Reader.End());
}
