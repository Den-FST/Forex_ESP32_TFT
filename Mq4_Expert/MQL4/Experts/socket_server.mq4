//+------------------------------------------------------------------+
//|                                             Expert Socket ESP32  |
//|                                              Copyright 2023, FST |
//|                                       http://www.fst.pt          |
//+------------------------------------------------------------------+

#property strict

#include <socket-library-mt4-mt5.mqh>

// Server socket
ServerSocket * glbServerSocket;

// Array of current clients
ClientSocket * glbClients[];

// Watch for need to create timer;
bool glbCreatedTimer = false;


#define COMPILE_AS_EA


// ---------------------------------------------------------------------
// If we are compiling as a script, then we want the script
// to display the inputs page
// ---------------------------------------------------------------------

#ifndef COMPILE_AS_EA
#property show_inputs
#endif


// ---------------------------------------------------------------------
// User-configurable parameters
// ---------------------------------------------------------------------

// Purely cosmetic; causes MT4 to display the comments below
// as the options for the input, instead of using a boolean true/false
//enum AllowedAddressesEnum {
//   aaeLocal = 0,   // localhost only (127.0.0.1)
//   aaeAny = 1      // All IP addresses
//};

// User inputs
input int                     PortNumber = 5000;     // TCP/IP port number
//input AllowedAddressesEnum    AcceptFrom = aaeLocal;  // Accept connections from

// --------------------------------------------------------------------
// Initialisation - set up server socket
// --------------------------------------------------------------------

void OnInit()
{
   // Create the server socket
   glbServerSocket = new ServerSocket(PortNumber, false);
   if (glbServerSocket.Created()) {
      Print("Server socket created");

      // Note: this can fail if MT4/5 starts up
      // with the EA already attached to a chart. Therefore,
      // we repeat in OnTick()
      glbCreatedTimer = EventSetMillisecondTimer(100);
   } else {
      Print("Server socket FAILED - is the port already in use?");
   }
}


// --------------------------------------------------------------------
// Termination - free server socket and any clients
// --------------------------------------------------------------------

void OnDeinit(const int reason)
{
   glbCreatedTimer = false;

   // Delete all clients currently connected
   for (int i = 0; i < ArraySize(glbClients); i++) {
      delete glbClients[i];
   }

   // Free the server socket
   delete glbServerSocket;
   Print("Server socket terminated");
}

// --------------------------------------------------------------------
// Timer - accept new connections, and handle incoming data from clients
// --------------------------------------------------------------------

void OnTimer()
{
   string recvMsg = "No data recived";
   // Keep accepting any pending connections until Accept() returns NULL
   ClientSocket * pNewClient = NULL;

   do {
      pNewClient = glbServerSocket.Accept();
      if (pNewClient != NULL) {
         int sz = ArraySize(glbClients);
         ArrayResize(glbClients, sz + 1);
         glbClients[sz] = pNewClient;

         Print("Connection recived!");
         //pNewClient.Send("Connected to the MT5 Server!\r\n");//---
         
      }

   } while (pNewClient != NULL);

   // Read incoming data from all current clients, watching for
   // any which now appear to be dead
   int ctClients = ArraySize(glbClients);
   for (int i = ctClients - 1; i >= 0; i--) {
      ClientSocket * pClient = glbClients[i];
      //pNewClient.Send("Hellowwwwww\r\n");


      // Keep reading CRLF-terminated lines of input from the client
      // until we run out of data
      string strCommand;
      do {
         strCommand = pClient.Receive();

         if (strCommand != "") Print(strCommand);

         // Free the server socket
         if (strCommand == "a"){
            delete glbServerSocket;
            Print("Server socket terminated");
          }
          if (strCommand == "b"){
            //pClient.Send(Symbol() + "," + DoubleToString(SymbolInfoDouble(Symbol(), SYMBOL_BID), 6) + "," + DoubleToString(SymbolInfoDouble(Symbol(), SYMBOL_ASK), 6) + "\r");
             Print("send client");
            
             double todayTradesResult;
            
                // Get the current server time
             datetime serverTime = TimeCurrent();
         
             // Get the day of the week (0 - Sunday, 1 - Monday, ..., 6 - Saturday)
             int dayOfWeek = TimeDayOfWeek(serverTime);
         
             int hours = TimeHour(serverTime);
             int minutes = TimeMinute(serverTime);
             
             string hrs = IntegerToString(hours);
             string mnts = IntegerToString(minutes);
             
             if (minutes < 10) {
             mnts = "0" + mnts;
             }
             
             if (hours <10) {
             hrs = "0" + hrs;
             }
             
             string hrs_mins = hrs + ":" + mnts;
            
            
            for (int i = 0; i < OrdersTotal(); i++)
                {
               if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES))
               {
               
               
                
 
                
                  // Get the current server time
                  datetime currentTime = TimeCurrent();
                         
               
                  // Get the order's open time
                  datetime orderOpenTime = OrderOpenTime();
            
            
                  // Calculate the time difference in seconds
                  int timeDifferenceSeconds = (int)(currentTime - orderOpenTime);
            
                  // Convert the time difference to a human-readable format
                  string timeDifferenceString = TimeDifferenceToString(timeDifferenceSeconds);
                  Print("Time: ", timeDifferenceString);
                   
                   
                   
                  // Retrieve the position information
                  int ticket = OrderTicket();
                  string symbol = OrderSymbol();
                  
                  
                  double currentTrend = GetTrend(symbol); // Get the trend for the symbol

                   // You can use the currentTrend value to make decisions or display it in the terminal or on the chart.
                   Print("Symbol: ", symbol, " | Trend: ", currentTrend);
                  
                  string currProfit;
                  
                  string type = (OrderType() == OP_BUY) ? "Buy" : "Sell";
                  double lots = OrderLots();
                  double openPrice = OrderOpenPrice();
                  float profit = OrderProfit();
                  double currentProfit = AccountProfit(); //-188.5 1 234 5 6

                  todayTradesResult = todayTradesCalc();
                  todayTradesResult *= 0.9;
                  //---Print("Current : ", todayTradesResult);
                 // int orderProfitInteger = (int)MathRound(profit);
                  double accbalance = AccountBalance();
                  //--- double totalProfit = CalculateTotalOpenOrdersProfit();
                  // Format the position information as a string
                  //string positionData = ticket + "," + symbol + "," + type + "," + DoubleToString(lots) + "," + DoubleToString(openPrice) + "," + DoubleToString(profit);
                  string positionData = symbol + "," + type + "," + DoubleToString(lots,2) + "," + DoubleToString(profit,1 ) + "," + DoubleToString(currentProfit, 1) + "," + DoubleToString(todayTradesResult, 1) + "," + timeDifferenceString + "," + hrs_mins + "," + dayOfWeek + "," + DoubleToString(currentTrend, 1) + ",";
                  pClient.Send(positionData  + StringLen(positionData) +  "#");
                  Print(positionData, StringLen(positionData));
                  Print("Account Balance: ", accbalance);
                  // Send the position data to the client
                  //int result = SocketSend(clientSocket, positionData, StringLen(positionData), 0);
                  
                  

               }
              // pClient.Send(positionData  + "\r");
               //Print(positionData, StringLen(positionData));
               }
               //pClient.Send("/r");
            
            Print("Today profit: ", todayTradesCalc(), " Regulated: todayTradesResult: ", todayTradesResult);
            
            
          }
 
      } while (strCommand != "");

      if (!pClient.IsSocketConnected()) {
         // Client is dead. Remove from array
         delete pClient;
         for (int j = i + 1; j < ctClients; j++) {
            glbClients[j - 1] = glbClients[j];
         }
         ctClients--;
         ArrayResize(glbClients, ctClients);
      }
   }
}



double GetTrend(string symbol)
{
    int period = 5; // Adjust this value for the desired period of the SMA
    double ma = iMA(symbol, 0, period, 0, MODE_SMA, PRICE_CLOSE, 0);
    double prev_ma = iMA(symbol, 0, period, 0, MODE_SMA, PRICE_CLOSE, 1);

    if (ma > prev_ma)
        return 1; // Uptrend
    else if (ma < prev_ma)
        return -1; // Downtrend
    else
        return 0; // No clear trend
}


//+------------------------------------------------------------------+
//| Calculate profit from orders closed today                        |
//+------------------------------------------------------------------+

double todayTradesCalc()
{


   double profit = 0;
   int totalOrders = OrdersHistoryTotal();
   
   for(int i = 0; i < totalOrders; i++)
     {
      if(OrderSelect(i, SELECT_BY_POS, MODE_HISTORY))
        {
         // Check if the order's close date is today
         if(TimeDay(OrderCloseTime()) == TimeDay(TimeCurrent()) &&
            TimeMonth(OrderCloseTime()) == TimeMonth(TimeCurrent()) &&
            TimeYear(OrderCloseTime()) == TimeYear(TimeCurrent()))
           {
            profit += (OrderProfit() - OrderSwap() - OrderCommission());
            
           // Print("Trades profit today: ", OrderProfit(), "Swap ", OrderSwap(), "Comission: ", OrderCommission() );
           }
        }
     }
     

    //---    Print("Trades profit today: ", profit);
        
    return profit;
}


string CalculateOrderOpenTime()
{
    int totalOrders = OrdersHistoryTotal();
    
    // Get the current server time
    datetime currentTime = TimeCurrent();
             
    string timeDifferenceString;
    
    // Get the order's open time
    datetime orderOpenTime = OrderOpenTime();


    // Calculate the time difference in seconds
    int timeDifferenceSeconds = (int)(currentTime - orderOpenTime);

    // Convert the time difference to a human-readable format
    timeDifferenceString = TimeDifferenceToString(timeDifferenceSeconds);
    // Print("Time: ", timeDifferenceString);
     
    
    
   
    return timeDifferenceString; 
}

string TimeDifferenceToString(int timeDifferenceSeconds)
{
    string timeString;
    int hours = timeDifferenceSeconds / 3600;
    int days = hours / 24;
    int minutes = (timeDifferenceSeconds % 3600) / 60;

    //string timeString = IntegerToString(days) + "d " + IntegerToString(hours) + "h " ;
    
    if (hours < 24 && hours > 0)
      {
      timeString = IntegerToString(hours) + "h ";
      } 
      else if(hours == 0)  {
      timeString = IntegerToString(minutes) + "m ";
      }
      else {
      timeString = IntegerToString(days) + "d ";
      }
    return timeString;
}


// Use OnTick() to watch for failure to create the timer in OnInit()
void OnTick()
{
   if (!glbCreatedTimer) glbCreatedTimer = EventSetMillisecondTimer(100);
}
