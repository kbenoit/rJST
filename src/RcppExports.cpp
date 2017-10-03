// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <RcppArmadillo.h>
#include <Rcpp.h>

using namespace Rcpp;

// jstcpp
int jstcpp(string model_status_str, Rcpp::List corpus, Rcpp::List sentiLexList, int numSentiLabs, int numTopics, int numiters, int updateParaStep, double alpha_, double beta_, double gamma_);
RcppExport SEXP _rJST_jstcpp(SEXP model_status_strSEXP, SEXP corpusSEXP, SEXP sentiLexListSEXP, SEXP numSentiLabsSEXP, SEXP numTopicsSEXP, SEXP numitersSEXP, SEXP updateParaStepSEXP, SEXP alpha_SEXP, SEXP beta_SEXP, SEXP gamma_SEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< string >::type model_status_str(model_status_strSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type corpus(corpusSEXP);
    Rcpp::traits::input_parameter< Rcpp::List >::type sentiLexList(sentiLexListSEXP);
    Rcpp::traits::input_parameter< int >::type numSentiLabs(numSentiLabsSEXP);
    Rcpp::traits::input_parameter< int >::type numTopics(numTopicsSEXP);
    Rcpp::traits::input_parameter< int >::type numiters(numitersSEXP);
    Rcpp::traits::input_parameter< int >::type updateParaStep(updateParaStepSEXP);
    Rcpp::traits::input_parameter< double >::type alpha_(alpha_SEXP);
    Rcpp::traits::input_parameter< double >::type beta_(beta_SEXP);
    Rcpp::traits::input_parameter< double >::type gamma_(gamma_SEXP);
    rcpp_result_gen = Rcpp::wrap(jstcpp(model_status_str, corpus, sentiLexList, numSentiLabs, numTopics, numiters, updateParaStep, alpha_, beta_, gamma_));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_rJST_jstcpp", (DL_FUNC) &_rJST_jstcpp, 10},
    {NULL, NULL, 0}
};

RcppExport void R_init_rJST(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
