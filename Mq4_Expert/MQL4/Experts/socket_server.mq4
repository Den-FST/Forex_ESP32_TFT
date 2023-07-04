// ###################################################################
// Based on the awesome example from: https://www.mql5.com/en/blogs/post/706665
// ###################################################################

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
            
            for (int i = 0; i < OrdersTotal(); i++)
                {
               if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES))
               {
                  // Retrieve the position information
                  int ticket = OrderTicket();
                  string symbol = OrderSymbol();
                  string type = (OrderType() == OP_BUY) ? "Buy" : "Sell";
                  double lots = OrderLots();
                  double openPrice = OrderOpenPrice();
                  float profit = OrderProfit();
                  double currentProfit = AccountProfit();
                  Print("Current profit: ", currentProfit);
                 // int orderProfitInteger = (int)MathRound(profit);
                  double accbalance = AccountBalance();
                  double totalProfit = CalculateTotalOpenOrdersProfit();
                  // Format the position information as a string
                  //string positionData = ticket + "," + symbol + "," + type + "," + DoubleToString(lots) + "," + DoubleToString(openPrice) + "," + DoubleToString(profit);
                  string positionData = symbol + "," + type + "," + DoubleToString(lots,2) + "," + DoubleToString(profit,1 ) + "," + DoubleToString(currentProfit, 1) + ",";
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


double CalculateTotalOpenOrdersProfit()
{
    double totalProfit = 0.0;
    
    // Get the total number of open orders
    int totalOrders = OrdersTotal();
    
    // Iterate through each open order
    for (int i = 0; i < totalOrders; i++)
    {
        // Select the order by its index
        if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES))
        {
            // Check if the order is a buy or sell order
            if (OrderType() == OP_BUY || OrderType() == OP_SELL)
            {
                // Calculate the profit of the order
                double orderProfit = OrderProfit();
                
                // Add the profit to the total
                totalProfit += orderProfit;
            }
        }
    }
    
    return totalProfit;
}
// Use OnTick() to watch for failure to create the timer in OnInit()
void OnTick()
{
   if (!glbCreatedTimer) glbCreatedTimer = EventSetMillisecondTimer(100);
}
