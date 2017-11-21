% Suppose you have model 'sub' nested inside model 'full' and you want to 
% test the statistical significance of the explanatory variables which
% appear in 'full' but not 'sub'.

% Suppose Y (column vector) is the dependent variable and Xfull, Xsub the 
% matrices of explanatory variables for the full and nested models.

% Run the regressions and store the coefficients, residuals and the tables
% of statistics:
[Bfull,~,Rfull,~,STATSfull] = regress(Y,Xfull);
[Bsub,~,Rsub,~,STATSsub] = regress(Y,Xsub);

% Compute the sums of squares of the residuals from both models:
SSRESfull = sum(Rfull.^2);
SSRESsub = sum(Rsub.^2);

% Calculate the degrees of freedom for the test:
% Difference between the numbers of parameters for the full and reduced
% model:
df1 = size(Xfull,2) - size(Xsub,2);
% Difference between the number of observations and the number of
% parameters for the full model:
df2 = size(Y,1) - size(Xfull,2);

% Compute the F-statistic:
Fstat = ((SSRESfull - SSRESsub)./df1)./(SSRESfull./df2);

% Get the critical value of the F distribution at (e.g.) the alpha=0.05 
% significance level:
CritVal = finv(1-0.05,df1,df2);

% If Fstat > CritVal then the variables which are in the full model but not
% the reduced one are jointly significant at the alpha=0.05 level.