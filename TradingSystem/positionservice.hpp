/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <map>
#include "soa.hpp"
#include "tradebookingservice.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position() = default;
  Position(const T& _productId) : product(_productId) {};

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the position quantity
  long GetPosition(string& _book) { return bookpositions[_book]; };

  // Get the aggregate position
  long GetAggregatePosition() {
    long aggposition = 0;
    for (auto it = bookpositions.begin(); it != bookpositions.end(); ++it)
    {
      aggposition += it->second;
    }
    return aggposition;
  };

  //  send position to risk service through listener
  void AddPosition(string& _book, long _position) 
  {
    if (bookpositions.find(_book) == bookpositions.end())
    {
      bookpositions.insert(pair<string,long>(_book, _position));
    }
    else
    {
      bookpositions[_book] += _position;
    }
  }

  // reload printer
  template<typename U>
  friend ostream& operator<<(ostream& os, const Position<U>& _position) 
  {
    T product = _position.GetProduct();
    string productId = product.GetProductId();
    vector<string> positions;

    for (const auto& p : _position.bookpositions) {
        positions.push_back(p.first);
        positions.push_back(std::to_string(p.second));
    }

    vector<string> components;
    components.push_back(productId);
    components.insert(components.end(), positions.begin(), positions.end());

    string combinedStr = join(components, ",");
    os << combinedStr;
    return os;
  };

private:
  T product;
  map<string,long> bookpositions;

};



// forward declaration of listener
template<typename T>
class PositionServiceListener;

/**
 * PositionService to manage positions across multiple books and securities.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PositionService : public Service<string, Position <T>>
{

public:
  // ctor and dtor
  PositionService() 
  {
    positionlistener = new PositionServiceListener<T>(this);
  };
  ~PositionService() = default;

  // Get data on our service given a key
  Position<T>& GetData(string _key) { return positions[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(Position<T>& _data) {};

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service.
  void AddListener(ServiceListener<Position<T>>* _listener) { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector<ServiceListener<Position<T>>*>& GetListeners() const { return listeners; };

  // Get the listener of the service
  PositionServiceListener<T>* GetPositionListener() { return positionlistener; };

  // Add a trade to the service
  void AddTrade(const Trade<T>& _trade) 
  {
    T product = _trade.GetProduct();
    string productId = product.GetProductId();
    string book = _trade.GetBook();
    long quantity = (_trade.GetSide() == BUY) ? _trade.GetQuantity() : -_trade.GetQuantity();
    if (positions.find(productId) == positions.end())
    {
      Position<T> position(product);
      position.AddPosition(book, quantity);
      positions.insert(pair<string,Position<T>>(productId, position));
    }
    else
    {
      positions[productId].AddPosition(book, quantity);
    }

    for (auto& listener: listeners)
    {
      listener -> ProcessAdd(positions[productId]);
    }

  };

private:
  map<string,Position<T>> positions;
  vector<ServiceListener<Position<T>>*> listeners;
  PositionServiceListener<T>* positionlistener;

};


/**
 * PositionServiceListener to subscribe data from tradebooking service
 * Type T is the product type.
 */
template<typename T>
class PositionServiceListener : public ServiceListener<Trade<T>>
{
private:
  PositionService<T>* positionservice;

public:
  // ctor and dtor
  PositionServiceListener(PositionService<T>* _positionservice) : positionservice(_positionservice) {};
  ~PositionServiceListener() = default;

  // Listener callback to process an add event to the Service
  void ProcessAdd(Trade<T>& _data) { positionservice -> AddTrade(_data); };

  // Listener callback to process a remove event to the Service
  void ProcessRemove(Trade<T>& _data) {};

  // Listener callback to process an update event to the Service
  void ProcessUpdate(Trade<T>& _data) {};

};



#endif
