/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"
#include "functions.hpp"

/**
 * PV01 risk.
 * Type T is the product type.
 */
template<typename T>
class PV01
{

public:

  // ctor for a PV01 value
  PV01() = default;
  PV01(const T& _product, double _pv01, long _quantity) : 
    product(_product), pv01(_pv01), quantity(_quantity) {};

  // Get the product on this PV01 value
  const T& GetProduct() const { return product; };

  // Get the PV01 value
  double GetPV01() const { return pv01; };

  // Get the quantity that this risk value is associated with
  long GetQuantity() const { return quantity; };

  // Add quantity associated with this risk value
  void AddQuantity(long _quantity) { quantity += _quantity; };

  // reload printer
  template<typename U>
  friend ostream& operator<<(ostream& os, const PV01<U>& pv01) 
  {
    T product = pv01.GetProduct();
    string _product = product.GetProductId();
    string _pv01 = to_string(pv01.GetPV01());
    string _quantity = to_string(pv01.GetQuantity());

    vector<string> components = {_product, _pv01, _quantity};
    string combinedStr = join(components, ",");
    os << combinedStr;
    return os;
  };

private:
  T product;
  double pv01;
  long quantity;

};


/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template<typename T>
class BucketedSector
{
public:
  // ctor for a bucket sector
  BucketedSector() = default;
  BucketedSector(const vector<T>& _products, string _name) :
    products(_products), name(_name) {};

  // Get the products associated with this bucket
  const vector<T>& GetProducts() const { return products; };

  // Get the name of the bucket
  const string& GetName() const { return name; };

private:
  vector<T> products;
  string name;

};


// forward declaration of RiskServiceListener
template<typename T>
class RiskServiceListener;

/**
 * Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class RiskService : public Service<string, PV01 <T>>
{
public:
  // ctor and dtor
  RiskService() 
  {
    riskservicelistener = new RiskServiceListener<T>(this);
  };
  ~RiskService() = default;

  // Get data on our service given a key
  PV01<T>& GetData(string _key) { return pv01s[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(PV01<T>& _data) {};

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service.
  void AddListener(ServiceListener<PV01<T>>* _listener) { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector< ServiceListener<PV01<T>>*>& GetListeners() const { return listeners; };

  // Get the special listener for risk service
  RiskServiceListener<T>* GetRiskServiceListener() { return riskservicelistener; };

  // Add a position that the service will risk
  void AddPosition(Position<T>& _position) 
  {
    T product = _position.GetProduct();
    string productId = product.GetProductId();
    long quantity = _position.GetAggregatePosition();
    double pv01Val = getPV01(productId);

    // create a PV01 object and publish it to the service
    PV01<T> pv01(product, pv01Val, quantity);
    if (pv01s.find(productId) != pv01s.end())
    {
      pv01s[productId].AddQuantity(quantity);
    }
    else
    {
      pv01s.insert(pair<string, PV01<T>>(productId, pv01));
    }

    // flow data to listener
    for (auto& listener : listeners)
      listener -> ProcessAdd(pv01);
  };

  // Get the bucketed risk for the bucket sector
  const PV01<BucketedSector<T>>& GetBucketedRisk(const BucketedSector<T>& _sector) const
  {
    vector<T>& products = _sector.GetProducts();
    double pv01BucketVal = 0.0;
    long quantity = 0;

    for (auto& product : products) {
      string productId = product.GetProductId();
      if (pv01s.find(productId) != pv01s.end())
      {
        pv01BucketVal += pv01s[productId].GetPV01() * pv01s[productId].GetQuantity();
        quantity += pv01s[productId].GetQuantity();
      }
    }

    return PV01<BucketedSector<T>>(_sector, pv01BucketVal, quantity);;
  };

private:
  vector<ServiceListener<PV01<T>>*> listeners;
  map<string, PV01<T>> pv01s;
  RiskServiceListener<T>* riskservicelistener;
};


/**
* Risk Service Listener subscribing data from Position Service to Risk Service.
* Type T is the product type.
*/
template<typename T>
class RiskServiceListener : public ServiceListener<Position<T>>
{
private:
  RiskService<T>* riskservice;

public:
  // ctor and dtor
  RiskServiceListener(RiskService<T>* _riskservice) : riskservice(_riskservice) {};
  ~RiskServiceListener() = default;

  // Listener callback to process an add event to the Service
  void ProcessAdd(Position<T>& _data) { riskservice -> AddPosition(_data); };

  // Listener callback to process a remove event to the Service
  void ProcessRemove(Position<T>& _data) {};

  // Listener callback to process an update event to the Service
  void ProcessUpdate(Position<T>& _data) {};

};


#endif
