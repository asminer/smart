
/*
    Main program for integer constraint programming.

    Basically, our job is to
      (1)  Initialize modules
      (2)  Process command-line arguments
      (3)  Start the parser
      (4)  Cleanup
*/

#include "../Options/optman.h"
#include "../Options/options.h"
#include "../Utils/library.h"
#include "../Utils/initializer.h"
#include "../ExprLib/expr.h"
#include "../ExprLib/engine.h"
#include "../ParseICP/parse_icp.h"


// #define DEBUG_MSRS
// #define DEBUG_OPTIONS

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

void setOption(const char* opt, const char* setting)
{
    option* O = option_manager::global().FindOption(opt);
    if (!O) {
#ifdef DEBUG_OPTIONS
        std::cerr << "Didn't find option " << opt << "\n";
#endif
        return;
    }
    option_enum* S = O->FindConstant(setting);
    if (!S) {
#ifdef DEBUG_OPTIONS
        std::cerr << "Didn't find setting " << setting << " for " << opt << "\n";
#endif
        return;
    }
#ifdef DEBUG_OPTIONS
    option::error err = O->SetValue(S);
    std::cerr << "Set " << opt << " to " << setting << "; error code: ";
    switch (err) {
        case option::Success:       std::cerr << "success\n";       return;
        case option::WrongType:     std::cerr << "wrong type\n";    return;
        case option::RangeError:    std::cerr << "range error\n";   return;
        case option::NullFunction:  std::cerr << "null func\n";     return;
        case option::Finalized:     std::cerr << "finalized\n";     return;
        case option::Duplicate:     std::cerr << "duplicate\n";     return;
        default:                    std::cerr << "unknown\n";
    }
#else
    O->SetValue(S);
#endif
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

    // Finalize registries
    type::finalizeRegistry();
    engtype::finalizeAll(option_manager::global());
    option_manager::global().DoneAddingOptions();

    //
    // Set default options
    //
    setOption("MinExpr", "EXPLICIT");
    setOption("MaxExpr", "EXPLICIT");
    setOption("SatExpr", "EXPLICIT");


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
