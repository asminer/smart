
/*
    Main program for integer constraint programming.

    Basically, our job is to
      (1)  Initialize modules
      (2)  Process command-line arguments
      (3)  Start the parser
      (4)  Cleanup
*/

#include "../Options/optman.h"
#include "../Utils/library.h"
#include "../Utils/initializer.h"
#include "../ExprLib/expr.h"
#include "../ParseICP/parse_icp.h"


// #define DEBUG_MSRS

// ============================================================

void SolveMeasures(outputStream& s, parse_module* pm)
{
    pm->Finish();
    traverse_data x(traverse_data::Compute);
    result answer;
    x.answer = &answer;

#ifdef DEBUG_MSRS
    s << "Measures to solve:\n";
    for (int i=0; i<pm->num_measures; i++) {
        s << "\t" << pm->measure_names[i] << " : ";
        if (pm->measure_calls[i])
            pm->measure_calls[i]->Print(s.stream());
        else
            s << "null";
        s << "\n";
    }
#endif

    for (int i=0; i<pm->num_measures; i++) {
        if (!pm->measure_calls[i]) continue;
        SafeCompute(pm->measure_calls[i], x);
        const type* t = pm->measure_calls[i]->Type();
        s << pm->measure_names[i] << ": ";
        t->print(s.stream(), answer);
        s << "\n";
    }
    // solve measures here...
}

int main(int argc, const char** argv, const char** env)
{
    //
    // Initialize modules
    //
    initializer::execute_all();

    // Parser initialization
    parse_module pm;
    pm.Initialize();

    // Finalize types
    type::finalizeRegistry();

    //
    // Process command line, start parser
    //

    outputStream& out = outputStream::globalOut();

    int code = 0;
    if (argc < 2) {
        // ==================================================================
        out << "\nICP version 0.1\n";
        out << "\nSupporting libraries:\n";
        library::printLibraryVersions(out.stream());
        out << "\n";
        out << "Usage : \n";
        out << "icp <file1> <file2> ... <filen>\n";
        out << "      Use the filename `-' to denote standard input\n";
        out << "\n";
        // ==================================================================
    } else {
        code = pm.ParseICPFiles(argv+1, argc-1);
        if (0==code) {
            SolveMeasures(out, &pm);
        }
    }

    //
    // Cleanup
    //

    return 0;
}
