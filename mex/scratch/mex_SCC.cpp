using namespace std;

#include "../src/graph.cpp"

/****************************/

/* The gateway function */
void mexFunction( int nlhs, mxArray *plhs[],
        int nrhs, const mxArray *prhs[]){
    
    /* Check for proper number of arguments */
    if (nrhs != 1) {
        mexErrMsgIdAndTxt("MATLAB:triangulate:nargin",
                "least-squares requires three input arguments.");}
    else if (nlhs != 3) {
        mexErrMsgIdAndTxt("MATLAB:triangulate:nargout",
                "least-squares requires two output argument.");}
    
    /* create a pointer to the real data in the input matrix  */
    double *G;  /* N*N*/
    G = mxGetPr(prhs[0]);
    
    /* get dimensions of the input matrix */
    size_t nrows,ncols; /* matrix dimensions */
    nrows = mxGetM(prhs[0]);
    ncols = mxGetN(prhs[0]);
    
    /****************************/
    
    /* get sparse graph Gs from dense/mex G */
    vector<T> tripletList;
    int i,j;
    j = 0;
    while (j<nrows){
        i = 0;
        while (i<ncols){
            if (G[j*ncols+i]>0){
                tripletList.push_back(T(i,j,G[j*ncols+i]));}
            i++;}
        j++;}
    Eigen::SparseMatrix<int> Gs(ncols,ncols);
    Gs.setFromTriplets(tripletList.begin(), tripletList.end());
    
    /****************************/
    
    /* construct the graph */
    Graph graph(ncols);
    
    /* fill in the graph */
    int k = 0;
    while (k<Gs.outerSize()){
        for (Eigen::SparseMatrix<int>::InnerIterator it(Gs,k);it;++it){
            graph.addEdge((int)it.row(),(int)it.col());}
        k++;}
    
    /* find the Strongest Connected Componenets (SCC) */
    graph.SCC();
    
    /****************************/
    
    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    double *SS = mxGetPr(plhs[0]);
    plhs[1] = mxCreateDoubleMatrix(1, graph.CC.size(), mxREAL);
    double *CC = mxGetPr(plhs[1]);
    plhs[2] = mxCreateDoubleMatrix(1, graph.JJ.size(), mxREAL);
    double *JJ = mxGetPr(plhs[2]);
    
    SS[0]=graph.SS;
    k = 0;
    while (k<graph.CC.size()) {
        CC[k] = graph.CC[k];
        k++;}
    
    k = 0;
    while (k<graph.JJ.size()) {
        JJ[k] = graph.JJ[k];
        k++;}
}