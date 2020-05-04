# include <iostream>
# include <math.h> 
# include <algorithm>
using namespace std;
   
void print_array(int* array, const int K)
{
    for(int i = 0; i < K; i++)
        cout << array[i] << " ";
    cout << "\n";
}
   
void input_value(int* array, const int K)
{
    for(int i = 0; i < K; i++)
        cin >> array[i];
}
   
int count_machine(int production, int limitPerMachine)
{
    int cnt = 0;
    while (cnt * limitPerMachine < production)
    {
        cnt++;
    }
    return cnt;
}
   
void allocate(int** production, const int* rawReturnRatio, const int excess, const int station, const int theDay, const int stationCnt)
{
    production[theDay][station] -= excess;
    production[theDay - 1][station] += excess;
   
    // int frontRoll = excess;
    // for(int i = station + 1; i <= stationCnt; i++)
    // {
    //     frontRoll = static_cast<int>(frontRoll / rawReturnRatio[i - 1]);
    //     if(production[theDay][i] - frontRoll >= 0)
    //         production[theDay][i] -= frontRoll;
    //     else
    //         production[theDay][i] = 0;
    //     production[theDay - 1][i] += frontRoll;
    // }
    int rollBack = excess;
    for(int i = station - 1; i >= 0; i--)
    {
        rollBack *= rawReturnRatio[i];
        if(production[theDay][i] - rollBack >= 0)
            production[theDay][i] -= rollBack;
        else
            production[theDay][i] = 0;
        production[theDay - 1][i] += rollBack;
    }
}
   
void allocate_bottleneck(int** production, const int* rawReturnRatio, const int* stationLimit, const int productionDay, const int stationCnt)
{   /*根據每個工作站產能限制現前分配各站產量*/
    for(int i = productionDay - 1; i > 0; i--) // 從最後一天往前找產量超過的點去往前配置
    { // 第一天surplus的話會有bug
        for(int j = stationCnt; j > 0; j--) // 原料不用判斷產能限制 station[0]是原料，所以長度是stationCnt+1
        { // 從完成品開始往前推
            if(production[i][j] > stationLimit[j - 1])
            { // 往上丟的工作站如果超過產能的話 迴圈會再往上丟
                int excess = production[i][j] - stationLimit[j - 1], // 剛剛這邊stationLimit裡面值給錯了
                    station = j, theDay = i; 
                allocate(production, rawReturnRatio, excess, station, theDay, stationCnt);
            }
        }
    }
}
   
void bestBuyRaw(int** production, const int* holdingCostPerDay, const int* rawPurchaseCost, const int productionDay)
{ /* 每個反轉點買足下個反轉點前的*/
    // 如果第一天買最便宜，不用判斷，買爆
    // 針對已經判斷過第一天存貨量的production
    int* tmpPurchase = new int[productionDay](); // 儲存該天要幫其他站購買的數量，再map進production
    for(int i = productionDay - 1; i > 0; i--)
    {
        int baseCost = rawPurchaseCost[i], bestDay = -1;
        for(int j = i - 1; j >= 0; j--) // 第i天的上一天
        {
            int dayDifference = i - j;
            if((holdingCostPerDay[0] * dayDifference) + rawPurchaseCost[j] < baseCost)
                bestDay = j;
        }
        if(bestDay != -1)
        {
            tmpPurchase[bestDay] += production[i][0];
            production[i][0] = 0;
        }
    }
    for(int i = 0; i < productionDay; i++)
    {
        production[i][0] += tmpPurchase[i];
    }
   
}
   
   
void allocate_inventory(int** production, int* inventory, const int* rawReturnRatio, const int productionDay, const int stationCnt, const int today)
{
   
    // if(today != 0) // 複製昨日的期末到期初，如果i是0的話，代表是第一天，就使用開機時的存貨
    // {
    //     for(int i = 0; i < stationCnt + 1; i++)
    //         inventory[today][i] = inventory[today - 1][i]; // 昨天的存貨為今天的期初存貨
    // }
   
    bool produceToday = true;
    int finalDemand = production[today][stationCnt];
    int finalInventory = inventory[stationCnt];
   
    if(finalInventory >= finalDemand) // 期初存貨足以滿足需求
        produceToday = false;
    if(produceToday)
    {
        production[today][stationCnt] = finalDemand - finalInventory;
        inventory[stationCnt] = 0; // 完成品存貨用完
        // rollBackProduction
        // for(int i = stationCnt - 1; i >= 0; i--) // update 完成該完成品需求個站所需的產量
        //     production[today][i] = production[today][i + 1] * rawReturnRatio[i];
        for(int i = stationCnt - 1; i >= 0; i--) // 判斷存貨是否足夠
        {
            production[today][i] = production[today][i + 1] * rawReturnRatio[i];
            if(inventory[i] >= production[today][i])// 存貨足以cover
            {
                inventory[i] -= production[today][i];
                production[today][i] = 0; // 不用生產
                // for(int j = i - 1; i >= 0; i--)
                //     production[today][j] = production[today][j + 1] * rawReturnRatio[j]; // 前面原料所需要再更新
            }
            else // 存貨不夠cover，把它用完
            {
                production[today][i] -= inventory[i]; // 要roll back 前面所需的產量
                inventory[i] = 0; // 用完存貨 
            }
            // rollBackProduction
            // for(int j = i - 1; j >= 0; j--)
            //     production[today][j] = production[today][j + 1] * rawReturnRatio[j]; // 前面原料所需要再更新
               
        }
    }
    else
    {
        inventory[stationCnt] -= finalDemand;
        for(int i = 0; i < stationCnt + 1; i++)
            production[today][i] = 0; // 當天都不用生產
    }
}
   
void to_output(int** production, int* limitPerMachine, const int productionDay, const int stationCnt)
{
    for(int i = 1; i < stationCnt + 1; i++)
    {
        for(int j = 0; j < productionDay; j++)
        {
            int machineUsed = count_machine(production[j][i], limitPerMachine[i - 1]); // 所以不是output浮點數的問題
            // int machineUsed = ceil(static_cast<double>(production[j][i]) / static_cast<double>(limitPerMachine[i - 1]));
            if(j != productionDay - 1)
                cout << machineUsed << " ";
            else
                cout << machineUsed;
        }
        cout << "\n";
    }
    // cout << "-------------------\n";
   
    for(int i = 1; i < stationCnt + 1; i++)
    {
        for(int j = 0; j < productionDay; j++)
        {
            if(j != productionDay - 1)
                cout << production[j][i] << " ";
            else
                cout << production[j][i];
        }
        cout << "\n";
    }
    // cout << "-------------------\n";
   
    for(int i = 0; i < productionDay; i++)
    {
        if(i != productionDay - 1)
            cout << production[i][0] << " ";
        else
            cout << production[i][0];
    }
   
}
int main()
{
    int stationCnt = 0, productionDay = 0;
    cin >> stationCnt >> productionDay;
   
    int* maxMachineCnt = new int[stationCnt](); // 每個工作站有幾台機器
    int* fixedCost = new int[stationCnt](); // 每個工作站機器開機的固定成本
    int* costPerUnit = new int[stationCnt](); // 每個工作站內機器每生產一公斤產品所需要的成本
    int* limitPerMachine = new int[stationCnt](); // 每個工作站內每台機器的產能上限
    int* rawReturnRatio = new int[stationCnt](); // 投入R公斤的原料會得到1公斤的產出
    int* holdingCostPerDay = new int[(stationCnt + 1)](); // 各工作站每日的存貨成本，0為原料存貨成本
    int* inventory = new int[stationCnt + 1]();  // 每日生產後剩餘，所以最初始在第一天生產完後會變動
    int* dailyDemand = new int[productionDay]();
    int* rawPurchaseCost = new int[productionDay](); // 每日購買原物料的成本
// --------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------
    input_value(maxMachineCnt, stationCnt);
    input_value(fixedCost, stationCnt);
    input_value(costPerUnit, stationCnt);
    input_value(limitPerMachine, stationCnt);
    input_value(rawReturnRatio, stationCnt);
    input_value(holdingCostPerDay, stationCnt + 1);
    input_value(inventory, stationCnt + 1); // 0: 原料 1: station1剩的 2: station2剩的
    input_value(dailyDemand, productionDay);
    input_value(rawPurchaseCost, productionDay);
// --------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------
    // cout << "maxMachineCnt: ";
    // print_array(maxMachineCnt, stationCnt);
    // cout << "fixedCost: ";
    // print_array(fixedCost, stationCnt);
    // cout << "costPerUnit: ";
    // print_array(costPerUnit, stationCnt);
    // cout << "limitPerMachine: ";
    // print_array(limitPerMachine, stationCnt);
    // cout << "rawReturnRatio: ";
    // print_array(rawReturnRatio, stationCnt);
    // cout << "holdingCostPerDay: ";
    // print_array(holdingCostPerDay, stationCnt+1);
    // cout << "inventory: "; // 初始
    // print_array(inventory, stationCnt + 1);
    // cout << "dailyDemand: ";
    // print_array(dailyDemand, productionDay);
    // cout << "rawPurchaseCost: ";
    // print_array(rawPurchaseCost, productionDay);
// --------------------------------------------------------------------------------------
    int* stationLimit = new int [stationCnt]; // 暫存每一站生產限制用
    for(int i = 0; i < stationCnt; i++)
        stationLimit[i] = maxMachineCnt[i] * limitPerMachine[i];
   
    // cout << "eachStationLimit: ";
    // print_array(stationLimit, stationCnt);
    // cout << "-------------------------------------------------" << "\n";
// --------------------------------------------------------------------------------------
    // int* totalDemand = new int[productionDay]();
    // for(int i = 0; i < productionDay; i++)
    //     totalDemand[productionDay - 1] += dailyDemand[i];
       
    // for(int i = productionDay -2; i >= 0; i--)
    //     totalDemand[i] = totalDemand[i + 1] * rawReturnRatio[i];
   
    // cout << "totalDemand produce in one single day: ";
    // print_array(totalDemand, productionDay);
   
    int** production = new int *[productionDay]; // 動態變動生產計畫
    for(int i = 0; i < productionDay; i++)
    {
        production[i] = new int[stationCnt + 1]();
        for(int j = stationCnt; j >= 0; j--)
        {
            if(j == stationCnt)
                production[i][j] = dailyDemand[i];
            else
                production[i][j] = production[i][j + 1] * rawReturnRatio[j];
        }
    }
   
    // cout << "production V1: \n";
    // for(int i = 0; i < productionDay; i++)
    //     print_array(production[i], stationCnt + 1);
    // cout << "-------------------------------------------------" << "\n";
   
// --------------------------------------------------------------------------------------
    /*分配掉所有初始存貨*/
    // cout << "allocate first day inventory: \n";
   
    // 不要註解掉
    allocate_bottleneck(production, rawReturnRatio, stationLimit, productionDay, stationCnt);
    for(int i = 0; i < productionDay; i++) // 將第一天的存貨都塞入生產規劃
        allocate_inventory(production, inventory, rawReturnRatio, productionDay, stationCnt, i);
   
    // cout << "production: \n";
    // for(int i = 0; i < productionDay; i++){
    //     cout << "Day" << i + 1 << ": ";
    //     print_array(production[i], stationCnt + 1);
    // }
   
    // totalDemand 垂直加總在productionDay = 1 的時候會runtime
    // 更新totalDemand，加入初始存貨後，每站共需要生產的量就沒那麼多了
    // for(int i = 0; i < productionDay; i++)
    //     totalDemand[i] = 0;
   
    // for(int i = 0; i < stationCnt + 1; i++)
    // {
    //     for(int j = 0; j < productionDay; j++)
    //         totalDemand[i] += production[j][i]; // 對欄做垂直加總
    // }
   
   
    // cout << "totalDemand after consider inventory: ";
    // print_array(totalDemand, stationCnt + 1);
    // cout << "-------------------------------------------------" << "\n";
   
//------------------------------------------------------------------------
    // cout << "allocate bottle neck again after consider inventory: \n";
   
    allocate_bottleneck(production, rawReturnRatio, stationLimit, productionDay, stationCnt);
   
    // cout << "production: \n";
    // for(int i = 0; i < productionDay; i++){
    //     cout << "Day" << i + 1 << ": ";
    //     print_array(production[i], stationCnt + 1);
    // }
   
    bestBuyRaw(production, holdingCostPerDay, rawPurchaseCost, productionDay);
       
    // cout << "production after raw allocation: \n";
    // for(int i = 0; i < productionDay; i++){
    //     cout << "Day" << i + 1 << ": ";
    //     print_array(production[i], stationCnt + 1);
    // }
// ------------------------------------------------------------------------
    to_output(production, limitPerMachine, productionDay, stationCnt);
   
    delete [] maxMachineCnt;
    delete [] fixedCost;
    delete [] costPerUnit;
    delete [] limitPerMachine;
    delete [] rawReturnRatio;
    delete [] holdingCostPerDay;
    delete [] inventory;
    delete [] dailyDemand;
    delete [] rawPurchaseCost;
    for(int i = 0; i < productionDay; i++)
        delete [] production[i];
    delete [] production;
    delete [] stationLimit;
    // delete [] totalDemand;
   
    return 0;
}