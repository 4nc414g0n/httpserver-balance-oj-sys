#ifndef HEADER
#include "header.cc"
#endif

class Test {
public:
    int getMost(vector<vector<int> > board){
        int dp[6][6];
        dp[0][0]=board[0][0];        
        for(int i=0;i<6;i++){
            for(int j=0;j<6;j++){
                if(!j && !i) continue;
                else dp[i][j] = max((j==0)?0:dp[i][j-1],(i==0)?0:dp[i-1][j])+ board[i][j];
            }
        }
        return dp[5][5];
    }
};
void Test1()
{
    vector<vector<int>> arr = {{426,306,641,372,477,409},
                               {223,172,327,586,363,553},
                               {292,645,248,316,711,295},
                               {127,192,495,208,547,175},
                               {131,448,178,264,207,676},
                               {655,407,309,358,246,714}};
    int bonus = Bonus().getMost(arr);
    int test = Test().getMost(arr);
    if(test!=bonus) cout<<"Example test1 Failed...."<<endl;
    if(test == bonus) cout<<"Example test1 Success!!!"<<endl;
}
void Test2()
{
    vector<vector<int>> arr = {{138,457,411,440,433,149},
                               {564,448,654,186,490,699},
                               {152,704,666,291,561,252},
                               {297,579,345,319,509,419},
                               {191,708,479,420,142,171},
                               {235,520,691,376,524,145}};
    int bonus = Bonus().getMost(arr);
    int test = Test().getMost(arr);
    if(test!=bonus) cout<<"Example test2 Failed...."<<endl;
    if(test == bonus) cout<<"Example test2 Success!!!"<<endl;
}
int main()
{

    Test1();
    Test2();
    return 0;
}