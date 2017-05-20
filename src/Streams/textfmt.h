
#ifndef TEXTFMT_H
#define TEXTFMT_H

class OutputStream;
class doc_formatter;

doc_formatter* MakeTextFormatter(int width, OutputStream &out);

#endif
