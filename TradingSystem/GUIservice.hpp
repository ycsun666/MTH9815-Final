/**
 * guiservice.hpp
 * Defines the data types and Service for GUI output.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */

#ifndef GUI_SERVICE_HPP
#define GUI_SERVICE_HPP

#include "soa.hpp"  
#include "functions.hpp"
#include "pricingservice.hpp"

// forward declaration of Connector and Listener
template<typename T>
class GUIConnector;
template<typename T>
class GUIServiceListener;

/**
* Service for outputing GUI with a certain throttle.
* Keyed on product identifier.
* Type T is the product type.
*/
template<typename T>
class GUIService : public Service<string, Price<T>>
{

public:
	// ctor and dtor
	GUIService() 
	{
		connector = new GUIConnector<T>(this);
		guilistener = new GUIServiceListener<T>(this);
		throttle = 300;
		cur_time = std::chrono::system_clock::now();
	};
	~GUIService() = default;

	// Get data from service
	Price<T>& GetData(string _key) override { return guis[_key]; };

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Price<T>& _data) override {};

	// Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
	void AddListener(ServiceListener<Price<T>>* _listener) override { listeners.push_back(_listener); };

	// Get all listeners on the Service
	const vector<ServiceListener<Price<T>>*>& GetListeners() const override { return listeners; };

	// Get the connector
	GUIConnector<T>* GetConnector() { return connector; };

	// Get the listener
	GUIServiceListener<T>* GetGUIServiceListener() { return guilistener; };

	// Get the throttle
	int GetThrottle() const { return throttle; };

	// Publish the throttled price
	void PublishThrottledPrice(Price<T>& _price)
	{
		// publish price to GUI if time is larger than throttle
		auto now = std::chrono::system_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - cur_time);
		if (diff.count() > throttle) {
			cur_time = now;
			connector -> Publish(_price);
		}
	};

private:

	map<string, Price<T>> guis;
	vector<ServiceListener<Price<T>>*> listeners;
	GUIConnector<T>* connector;
	GUIServiceListener<T>* guilistener;
	int throttle;
	std::chrono::system_clock::time_point cur_time;

};


/**
* GUI Connector publishing data from GUI Service.
* Type T is the product type.
*/
template<typename T>
class GUIConnector : public Connector<Price<T>>
{

public:
	// ctor and dtor
	GUIConnector(GUIService<T>* _service) : service(_service) {};
	~GUIConnector() = default;

	// Publish data to the Connector
	void Publish(Price<T>& _data) override 
	{
		ofstream file;
		file.open("../data/gui.txt", ios::app);
		file << getTimeStamp() << "," << _data << endl;
		file.close();
	};

	void Subscribe(ifstream &data) override {};

private:
	GUIService<T>* service;

};


/**
* GUI Service Listener subscribing data to GUI Data.
* Type T is the product type.
*/
template<typename T>
class GUIServiceListener : public ServiceListener<Price<T>>
{

public:
	// ctor and dtor
	GUIServiceListener(GUIService<T>* _guiservice) : guiservice(_guiservice) {};
	~GUIServiceListener() = default;

	// Listener callback to process an add event to the Service
	void ProcessAdd(Price<T>& _price) override { guiservice -> OnMessage(_price); };

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Price<T>& _price) override {};

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Price<T>& _price) override {};

private:
	GUIService<T>* guiservice;

};


#endif