/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */

#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"
#include "functions.hpp"

// Various inqyury states
enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:

  // ctor for an inquiry
  Inquiry() = default;
  Inquiry(string _inquiryId, const T& _product, Side _side, long _quantity, double _price, InquiryState _state) : 
    inquiryId(_inquiryId), product(_product), side(_side), quantity(_quantity), price(_price), state(_state) {}
  
  // Get the inquiry ID
  const string& GetInquiryId() const { return inquiryId; };

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the side on the inquiry
  Side GetSide() const { return side; };

  // Get the quantity that the client is inquiring for
  long GetQuantity() const { return quantity; };
 
  // Get the price that we have responded back with
  double GetPrice() const { return price; };

  // Set the price
  void SetPrice(double _price) { price = _price; };

  // Get the current state on the inquiry
  InquiryState GetState() const { return state; };

  // Set the current state on the inquiry
  void SetState(InquiryState _state) { state = _state; };

  // object printer
  template<typename U>
  friend ostream& operator<<(ostream& os, const Inquiry<U>& inquiry) {
    string inquiryId = inquiry.GetInquiryId();
    T product = inquiry.GetProduct();
    string productId = product.GetProductId();
    string side = inquiry.GetSide() == BID ? "BID" : "OFFER";

    long quantity = inquiry.GetQuantity();
    double price = inquiry.GetPrice();

    InquiryState state = inquiry.GetState();
    string stateStr;
    if (state == RECEIVED) {
        stateStr = "RECEIVED";
    } else if (state == QUOTED) {
        stateStr = "QUOTED";
    } else if (state == DONE) {
        stateStr = "DONE";
    } else if (state == REJECTED) {
        stateStr = "REJECTED";
    } else if (state == CUSTOMER_REJECTED) {
        stateStr = "CUSTOMER_REJECTED";
    }

    vector<string> components = {
        inquiryId, productId, side, to_string(quantity), convertPrice(price), stateStr
    };
    string combinedStr = join(components, ",");
    os << combinedStr;
    return os;
  };

private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price;
  InquiryState state;

};


// forward declaration of connector and inquiry listener
template<typename T>
class InquiryConnector;
template<typename T>
class InquiryServiceListener;


/**
 * Service for customer inquiry objects.
 * Keyed on inquiry identifier
 * Type T is the product type.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{

public:
  // ctor and dtor
  InquiryService() 
  {
    connector = new InquiryConnector<T>(this);
  };
  ~InquiryService() = default;

  // Get data on our service given a key
  Inquiry<T>& GetData(string _key) { return inquirys[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(Inquiry<T>& data) {
    InquiryState state = data.GetState();
    string inquiryId = data.GetInquiryId();

    if (state == RECEIVED) {
        // If inquiry is received, send back a quote to the connector via publish()
        connector->Publish(data);
    } else if (state == QUOTED) {
        // Finish the inquiry with DONE status and send an update of the object
        data.SetState(DONE);

        // Store the inquiry
        if (inquirys.find(inquiryId) != inquirys.end()) {
            inquirys.erase(inquiryId);
        }
        inquirys.insert(pair<string, Inquiry<T>>(inquiryId, data));
    }

    // If inquiry is done, remove it from the map
    if (data.GetState() == DONE) {
        inquirys.erase(data.GetInquiryId());
    }
    // Otherwise, update the inquiry
    else {
        inquirys[data.GetInquiryId()] = data;
    }

    for (auto& listener : listeners) {
        listener->ProcessAdd(data);
    }
}


  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service.
  void AddListener(ServiceListener<Inquiry<T>>* _listener) { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector< ServiceListener<Inquiry<T>>* >& GetListeners() const { return listeners; };

  // Get the connector
  InquiryConnector<T>* GetConnector() { return connector; };

  // Send a quote back to the client
  void SendQuote(const string &inquiryId, double price) {
    Inquiry<T>& inquiry = inquirys[inquiryId];
    // update the inquiry if received
    if (inquiry.GetState() == RECEIVED)
    {
      inquiry.SetPrice(price);
      for (auto& listener : listeners)
      {
        listener -> ProcessAdd(inquiry);
      }
    }
  };

  // Reject an inquiry from the client
  void RejectInquiry(const string &inquiryId) {
    Inquiry<T>& inquiry = inquirys[inquiryId];
    // update the inquiry
    inquiry.SetState(REJECTED);
  };

private:
  InquiryConnector<T>* connector;
  map<string, Inquiry<T>> inquirys;
  vector<ServiceListener<Inquiry<T>>*> listeners;

};


/**
* Inquiry Connector subscribing data to Inquiry Service and publishing data to Inquiry Service.
* Type T is the product type.
*/
template <typename T>
class InquiryConnector : public Connector<Inquiry<T>>
{
private:
  InquiryService<T>* service;

public:
  // ctor and dtor
  InquiryConnector(InquiryService<T>* _service) : service(_service) {};
  ~InquiryConnector() = default;

  // Publish data to the Connector
  // If subscribe-only, then this does nothing
  void Publish(Inquiry<T>& _data) override {
    if (_data.GetState() == RECEIVED) {
        _data.SetState(QUOTED);
        service -> OnMessage(_data);
        _data.SetState(DONE);
        service -> OnMessage(_data);
    }
  };

  // Subscribe data from connector
  void Subscribe(ifstream& _data) override {
    string line;
    while (getline(_data, line)) {
      // Parse the line into attributes
      vector<string> lineVec;
      stringstream ss(line);
      string attribute;
      while (getline(ss, attribute, ',')) {
          lineVec.push_back(attribute);
      }

      // Create and populate an Inquiry object
      string inquiryId = lineVec[0];
      string productId = lineVec[1];
      T product = getProductObject<T>(productId);
      Side side = lineVec[2] == "BUY" ? BUY : SELL;
      long quantity = stol(lineVec[3]);
      double price = convertPrice(lineVec[4]);
      InquiryState state = lineVec[5] == "RECEIVED" ? RECEIVED : 
                            lineVec[5] == "QUOTED" ? QUOTED : 
                            lineVec[5] == "DONE" ? DONE : 
                            lineVec[5] == "REJECTED" ? REJECTED : CUSTOMER_REJECTED;

      Inquiry<T> inquiry(inquiryId, product, side, quantity, price, state);

      // Pass the inquiry object to the service
      service->OnMessage(inquiry);
    }
  };
};


#endif
