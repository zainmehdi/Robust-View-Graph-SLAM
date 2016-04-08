/* Kernel density estimation by Tim Nugent (c) 2014
 * Based on Philipp K. Janert's Perl module:
 * http://search.cpan.org/~janert/Statistics-KernelEstimation-0.05
 * Multivariate stuff from here:
 * http://sfb649.wiwi.hu-berlin.de/fedc_homepage/xplore/ebooks/html/spm/spmhtmlnode18.html 
 */
#include "kde.hpp"

void KDE::calc_bandwidth(){
    for(curr_var = 0; curr_var < data_matrix.size(); curr_var++){
        if(bandwidth_map[curr_var] == -1.0){
            switch(bandwidth_opt_type){
                case 2:
                    optimal_bandwidth();
                    break;
                case 3:
                    optimal_bandwidth_safe();
                    break;
                default:
                    default_bandwidth();
                    break;}}}}

void KDE::set_bandwidth_opt_type(int x){
    if(kernel == 1){ bandwidth_opt_type = x; }}

void KDE::set_kernel_type(int x){
    kernel = x;
    if(kernel != 1){
        extension = 0;
        bandwidth_opt_type = 1;}}

double KDE::pdf(double x){
    vector<double> tmp;
    tmp.push_back(x);
    return(pdf(tmp));}

double KDE::pdf(double x, double y){
    vector<double> tmp;
    tmp.push_back(x);
    tmp.push_back(y);
    return(pdf(tmp));}

double KDE::pdf(vector<double>& data){
    calc_bandwidth();
    double d = 0.0;
    for(unsigned int i = 0; i < data_matrix[0].size(); i++){
        double a = 1.0;
        for(curr_var = 0; curr_var < data_matrix.size(); curr_var++){
            switch(kernel){
                case 2: a *= box_pdf(data[curr_var],data_matrix[curr_var][i],bandwidth_map[curr_var]);break;
                case 3: a *= epanechnikov_pdf(data[curr_var],data_matrix[curr_var][i],bandwidth_map[curr_var]);break;
                default: a *= gauss_pdf(data[curr_var],data_matrix[curr_var][i],bandwidth_map[curr_var]);break;}}
        d += a;}
    return(d/count_map[0]);}

double KDE::cdf(double x){
    vector<double> tmp;
    tmp.push_back(x);
    return(cdf(tmp));}

double KDE::cdf(double x, double y){
    vector<double> tmp;
    tmp.push_back(x);
    tmp.push_back(y);
    return(cdf(tmp));}

double KDE::cdf(vector<double>& data){
    calc_bandwidth();
    double d = 0.0;
    for(unsigned int i = 0; i < data_matrix[0].size(); i++){
        double a = 1.0;
        for(curr_var = 0; curr_var < data_matrix.size(); curr_var++){
            switch(kernel){
                case 2: a *= box_cdf(data[curr_var],data_matrix[curr_var][i],bandwidth_map[curr_var]);break;
                case 3: a *= epanechnikov_cdf(data[curr_var],data_matrix[curr_var][i],bandwidth_map[curr_var]);break;
                default: a *= gauss_cdf(data[curr_var],data_matrix[curr_var][i],bandwidth_map[curr_var]);break;}}
        d += a;}
    return(d/count_map[0]);}

void KDE::default_bandwidth(){
    if(!count_map[curr_var]){
        std::cout << "No data!" << std::endl;
        exit(1);}
    double x  = sum_x_map[curr_var]/count_map[curr_var];
    double x2 = sum_x2_map[curr_var]/count_map[curr_var];
    double sigma = sqrt(x2 - (x*x));
    double b = sigma*(pow((3.0*count_map[curr_var]/4.0),(-1.0/5.0)));
    if(bandwidth_opt_type == 1) bandwidth_map[curr_var] = b;
    default_bandwidth_map[curr_var] = b;}

// Secant method
void KDE::optimal_bandwidth(int maxiters, double eps){
    if(!count_map[curr_var]){
        std::cout << "No data!" << std::endl;
        exit(1);}
    default_bandwidth();
    double x0 = default_bandwidth_map[curr_var];
    double y0 = optimal_bandwidth_equation(x0,get_min(curr_var),get_max(curr_var),data_matrix[curr_var]);
    double x = 0.8*x0;
    double y = optimal_bandwidth_equation(x,get_min(curr_var),get_max(curr_var),data_matrix[curr_var]);
    int i = 0;
    while(i < maxiters){
        x -= y*(x0-x)/(y0-y);
        y = optimal_bandwidth_equation(x,get_min(curr_var),get_max(curr_var),data_matrix[curr_var]);
        if(abs(y) < eps*y0){
            break;}
        i++;}
    bandwidth_map[curr_var] = x;}

double KDE::optimal_bandwidth_equation(double w, double min, double max, vector<double>& data){
    double alpha = 1.0/(2.0*sqrt(M_PI));
    double sigma = 1.0;
    double n = count_map[curr_var];
    double q = stiffness_integral(w,min,max,data);
    return w - pow(((n*q*pow(sigma,4))/alpha ),(-1.0/5.0));}

/* Calculates the integral over the square of the curvature using the trapezoidal rule. decreases 
 * step size by half until the relative error in the last step is less eps. 
 */
double KDE::stiffness_integral(double w, double mn, double mx, vector<double>& data){
    double eps = 1e-4;
    double n = 1;
    double dx = (mx-mn)/n;
    double curv_mx = curvature(mx,w,data);
    double curv_mn = curvature(mn,w,data);
    double yy = 0.5*((curv_mx*curv_mx)+(curv_mn*curv_mn))*dx;
    double maxn = (mx-mn)/sqrt(eps);
    maxn = maxn > 2048 ? 2048 : maxn;
    for(int n = 2; n <= maxn; n *= 2){
        dx /= 2.0;
        double y = 0.0;
        for(int i = 1; i <= n-1; i +=2){
            curv_mn = pow(curvature(mn + i*dx, w, data),2);
            y += curv_mn;}
        yy = 0.5*yy + y*dx;
        if(n > 8 && abs(y*dx-0.5*yy) < eps*yy){
            break;}}
    return(yy);}

// Bisection method
void KDE::optimal_bandwidth_safe(double eps){
    if(!count_map[curr_var]){
        std::cout << "No data!" << std::endl;
        exit(1);}
    default_bandwidth();
    double x0 = default_bandwidth_map[curr_var]/count_map[curr_var];
    double x1 = 2*default_bandwidth_map[curr_var];
    double y0 = optimal_bandwidth_equation(x0,min_map[curr_var],max_map[curr_var],data_matrix[curr_var]);
    double y1 = optimal_bandwidth_equation(x1,min_map[curr_var],max_map[curr_var],data_matrix[curr_var]);
    if(y0 * y1 >= 0){
        //	std::cerr << "Interval [ f(x0=$x0)=$y0 : f(x1=$x1)=$y1 ] does not bracket root." << std::endl;
    }
    double x = 0.0, y = 0.0;
    int i = 0;
    while(abs(x0 - x1) > eps*x1){
        i += 1;
        x = (x0 + x1 )/2;
        y = optimal_bandwidth_equation(x,min_map[curr_var],max_map[curr_var],data_matrix[curr_var]);
        if(abs(y) < eps*y0){ break; }
        if(y * y0 < 0){ x1 = x; y1 = y;}
        else{ x0 = x; y0 = y; }}
    bandwidth_map[curr_var] = x;}

double KDE::curvature(double x, double w, vector<double>& data){
    double y = 0.0;
    std::vector<double>::iterator it;
    for(it = data.begin(); it != data.end(); it++){
        y += gauss_curvature(x,*it,w);}
    return(y/count_map[curr_var]);}

void KDE::add_data(double x){
    vector<double> tmp;
    tmp.push_back(x);
    add_data(tmp);}

void KDE::add_data(double x, double y){
    //std::cout << x << "," << y << std::endl;
    vector<double> tmp;
    tmp.push_back(x);
    tmp.push_back(y);
    add_data(tmp);}

void KDE::add_data(vector<double>& x){
    if(!data_matrix.size()){
        for(size_t i = 0; i < x.size(); i++){
            vector<double> tmp;
            tmp.push_back(x[i]);
            data_matrix.push_back(tmp);
            sum_x_map[i] = x[i];
            sum_x2_map[i] = x[i]*x[i];
            count_map[i] = 1;
            min_map[i] = x[i];
            max_map[i] = x[i];
            bandwidth_map[i] = -1.0;}}
    else{if(x.size() != data_matrix.size()){
        std::cout << "Number of variables doesn't match!" << std::endl;}
    else{for(size_t i = 0; i < x.size(); i++){
        data_matrix[i].push_back(x[i]);
        sum_x_map[i] += x[i];
        sum_x2_map[i] += x[i]*x[i];
        count_map[i]++;
        min_map[i] = x[i] < min_map[i] ? x[i] : min_map[i];
        max_map[i] = x[i] > max_map[i] ? x[i] : max_map[i];
        bandwidth_map[i] = -1.0;}}}}

double KDE::gauss_cdf(double x, double m, double s){
    // Abramowitz Stegun Normal Cumulative Distribution Function
    double z = abs(x - m)/s;
    double t = 1.0/(1.0 + 0.2316419*z);
    double y = t*( 0.319381530 + t*(-0.356563782 + t*(1.781477937 + t*(-1.821255978 + t*1.330274429 ))));
    if(x >= m){return 1.0 - gauss_pdf(x,m,s)*y*s;}
    else{return gauss_pdf(x,m,s)*y*s;}}

double KDE::gauss_pdf(double x, double m, double s){
    double z = (x - m)/s;
    return exp(-0.5*z*z)/(s*sqrt( 2.0*M_PI));}

double KDE::gauss_curvature(double x, double m, double s){
    double z = (x - m)/s;
    return ((z*z) - 1.0)*gauss_pdf(x,m,s)/(s*s);}

double KDE::box_pdf(double x, double m, double s){
    if(x < m-0.5*s || x > m+0.5*s){return (0.0);}
    else{return (1.0/s);}}

double KDE::box_cdf(double x, double m, double s){
    if(x < m-0.5*s){ return (0.0); }
    if(x > m+0.5*s){ return 1.0; }
    return (x-m)/s + 0.5;}

double KDE::epanechnikov_pdf(double x, double m, double s){
    double z = (x-m)/s;
    if(fabs(z) > 1.0){
        return (0.0);}
    return 0.75*(1.0-(z*z))/s;}

double KDE::epanechnikov_cdf(double x, double m, double s){
    double z = (x-m)/s;
    if(z < -1.0){ return (0.0); }
    if(z >  1.0){ return (1.0); }
    return 0.25*(2.0 + 3.0*z - (z*z*z));}


double KDE::RationalApproximation(double t){
    // Abramowitz and Stegun formula 26.2.23.
    // The absolute value of the error should be less than 4.5 e-4.
    double c[] = {2.515517, 0.802853, 0.010328};
    double d[] = {1.432788, 0.189269, 0.001308};
    return t - ((c[2]*t + c[1])*t + c[0]) /
            (((d[2]*t + d[1])*t + d[0])*t + 1.0);}
double KDE::NormalCDFInverse(double p){
    if (p <= 0.0 || p >= 1.0){
        std::stringstream os;
        os << "Invalid input argument (" << p
                << "); must be larger than 0 but less than 1.";
        throw std::invalid_argument( os.str() );}
    // See article above for explanation of this section.
    if (p < 0.5){// F^-1(p) = - G^-1(p)
        return -RationalApproximation( sqrt(-2.0*log(p)) );}
    else{// F^-1(p) = G^-1(1-p)
        return RationalApproximation( sqrt(-2.0*log(1-p)) );}}


/*
 * The standard normal CDF, for one random variable.
 *
 *   Author:  W. J. Cody
 *   URL:   http://www.netlib.org/specfun/erf
 *
 * This is the erfc() routine only, adapted by the transform stdnormal_cdf(u)=(erfc(-u/sqrt(2))/2;
 */
double KDE::stdnormal_cdf(double u){
    const double a[5] = {1.161110663653770e-002,3.951404679838207e-001,2.846603853776254e+001,
    1.887426188426510e+002,3.209377589138469e+003};
    const double b[5] = {1.767766952966369e-001,8.344316438579620e+000,1.725514762600375e+002,
    1.813893686502485e+003,8.044716608901563e+003};
    const double c[9] = {2.15311535474403846e-8,5.64188496988670089e-1,8.88314979438837594e00,
    6.61191906371416295e01,2.98635138197400131e02,8.81952221241769090e02,1.71204761263407058e03,
    2.05107837782607147e03,1.23033935479799725E03};
    const double d[9] = {1.00000000000000000e00,1.57449261107098347e01,1.17693950891312499e02,
    5.37181101862009858e02,1.62138957456669019e03,3.29079923573345963e03,4.36261909014324716e03,
    3.43936767414372164e03,1.23033935480374942e03};
    const double p[6] = {1.63153871373020978e-2,3.05326634961232344e-1,3.60344899949804439e-1,
    1.25781726111229246e-1,1.60837851487422766e-2,6.58749161529837803e-4};
    const double q[6] = {1.00000000000000000e00,2.56852019228982242e00,1.87295284992346047e00,
    5.27905102951428412e-1,6.05183413124413191e-2,2.33520497626869185e-3};
    register double y, z;
    if (isnan(u))
        return NAN;
    if (!isfinite(u))
        return (u < 0 ? 0.0 : 1.0);
        y = fabs(u);
        if (y <= 0.46875*M_SQRT2) {
            /* evaluate erf() for |u| <= sqrt(2)*0.46875 */
            z = y*y;
            y = u*((((a[0]*z+a[1])*z+a[2])*z+a[3])*z+a[4])
            /((((b[0]*z+b[1])*z+b[2])*z+b[3])*z+b[4]);
            return 0.5+y;}
        z = exp(-y*y/2)/2;
        if (y <= 4.0) {
            /* evaluate erfc() for sqrt(2)*0.46875 <= |u| <= sqrt(2)*4.0 */
            y = y/M_SQRT2;
            y = ((((((((c[0]*y+c[1])*y+c[2])*y+c[3])*y+c[4])*y+c[5])*y+c[6])*y+c[7])*y+c[8])
            /((((((((d[0]*y+d[1])*y+d[2])*y+d[3])*y+d[4])*y+d[5])*y+d[6])*y+d[7])*y+d[8]);
            y = z*y;}
        else {
            /* evaluate erfc() for |u| > sqrt(2)*4.0 */
            z = z*M_SQRT2/y;
            y = 2/(y*y);
            y = y*(((((p[0]*y+p[1])*y+p[2])*y+p[3])*y+p[4])*y+p[5])
            /(((((q[0]*y+q[1])*y+q[2])*y+q[3])*y+q[4])*y+q[5]);
            y = z*(M_1_SQRTPI-y);}
        return (u < 0.0 ? y : 1-y);
};

/*
 * The inverse standard normal distribution.
 *
 *   Author:      Peter John Acklam <pjacklam@online.no>
 *   URL:         http://home.online.no/~pjacklam
 *
 * This function is based on the MATLAB code from the address above, translated to C, and adapted 
 * for our purposes.
 */
double KDE::stdnormal_inv(double p){
    const double a[6] = {-3.969683028665376e+01,2.209460984245205e+02,-2.759285104469687e+02,
    1.383577518672690e+02,-3.066479806614716e+01,2.506628277459239e+00};
    const double b[5] = {-5.447609879822406e+01,1.615858368580409e+02,-1.556989798598866e+02,
    6.680131188771972e+01,-1.328068155288572e+01};
    const double c[6] = {-7.784894002430293e-03,-3.223964580411365e-01,-2.400758277161838e+00,
    -2.549732539343734e+00,4.374664141464968e+00,2.938163982698783e+00};
    const double d[4] = {7.784695709041462e-03, 3.224671290700398e-01,2.445134137142996e+00,
    3.754408661907416e+00};
    register double q, t, u;
    if (isnan(p) || p > 1.0 || p < 0.0)
        return NAN;
    if (p == 0.0)
        return -1.0/0.0;
    if (p == 1.0)
        return  1.0/0.0;
    q = MIN(p,1-p);
    if (q > 0.02425) {/* Rational approximation for central region. */
        u = q-0.5;
        t = u*u;
        u = u*(((((a[0]*t+a[1])*t+a[2])*t+a[3])*t+a[4])*t+a[5])
        /(((((b[0]*t+b[1])*t+b[2])*t+b[3])*t+b[4])*t+1);}
    else {/* Rational approximation for tail region. */
        t = sqrt(-2*log(q));
        u = (((((c[0]*t+c[1])*t+c[2])*t+c[3])*t+c[4])*t+c[5])
        /((((d[0]*t+d[1])*t+d[2])*t+d[3])*t+1);}
    /* The relative error of the approximation has absolute value less than 1.15e-9.  One iteration 
     * of Halley's rational method (third order) gives full machine precision... */
    t = stdnormal_cdf(u)-q;       /* error */
    t = t*M_SQRT2PI*exp(u*u/2);   /* f(u)/df(u) */
    u = u-t/(1+u*t/2);            /* Halley's method */
    return (p > 0.5 ? -u : u);
};