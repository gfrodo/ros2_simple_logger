#ifndef ENTITY_H
#define ENTITY_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <mutex>
#include <fstream>
#include <string>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/publisher.hpp"
#include "rclcpp/subscription.hpp"

#include <memory>
#include "Reflect.h"


using namespace std;
using namespace std::placeholders;
namespace KamaroModule
{
    
    class EntityBase {
    public:
	EntityBase(uint16_t _id, bool _subscribe, std::shared_ptr<rclcpp::node::Node> parentNode, std::string _className);
	
	/**
	 * @brief getId
	 * @return The Component ID
	 */
	int64_t getId()
	{
	    return id;
	}
	/**
	 * @brief getName
	 * @return The name of the current component
	 */
	std::string getName()
	{
	    return getClassName()  + std::to_string(id);
	}
	/**
	 * @brief getClassName
	 * @return The class name
	 */
	std::string getClassName()
	{
	    return className;
	}
	/**
	 * @brief isVirtual
	 * @return In case this is a virtual Entity (A Entity without a direct hardware representation) this will be true
	 */
	bool isVirtual()
	{
	    return virtualEntity;
	}
	/**
	 * @brief isSubscriber
	 * @return Is this a subscriber or a publisher
	 */
	bool isSubscriber()
	{
	    return subscriber;
	}
    protected:
	virtual std::string getAutogeneratedClassName();
	/**
	 * @brief parent
	 * The node -> we need to pass in order to create the publisher, subscription and parameter client
	 */
	std::shared_ptr<rclcpp::node::Node> parent;
	/**
	 * Helper method so we can use REFLECT on a variable
	 */
	template <class T>
	void addElement(string type,T &data){
	    internalmap.push_back( (Element*)new SpecificElement<T>(type , data));
	}
	/*
	 * Vector all elements that had used REFLECT on them are stored in
	 */
	std::vector<Element*> internalmap;
	
	/**
	 * Contains children of this entity -> used for building some kind of tree 
	 */
	std::vector<EntityBase*> childs;
	
    private:
	/**
	 * @brief id
	 * Component ID
	 */
	int64_t id;
	/**
	 * @brief virtualEntity
	 * Is it a real component
	 */
	bool virtualEntity;
	/**
	 * @brief subscriber
	 */
	bool subscriber;
	
	/**
	 * @brief Name of the implementing class
	 */
	std::string className;
    };
    
    template <typename MessageType>
    class Entity : public EntityBase
    {
	
    public:
	
	/**
	 * @brief Constructor of Entity
	 * @param className is used together with the id to itentify topics, etc. of this entity
	 */
	Entity(uint16_t _id, bool _subscribe, std::shared_ptr<rclcpp::node::Node> parentNode, std::string className) : EntityBase(_id, _subscribe, parentNode, className)
	{
	    //Some ROS2 QOS Configuration -> Taken from an example
	    custom_qos_profile = rmw_qos_profile_default;
	    custom_qos_profile.depth = 7;
	    
	    //Create a new parameterClient
	    //The client is used for storing meta information about a component
	    this->parameterClient = std::make_shared<rclcpp::parameter_client::AsyncParametersClient>(parent, "ParameterServer");
	    std::cout << "Created parameter client" << std::endl;
	    //Register the client on an event that is thrown in case a parameter has changed
	    parameterEventSubscription = parameterClient->on_parameter_event(std::bind(&Entity::onParameterEvent, this, _1));
	    std::cout << "Registered event on parameter client" << std::endl;
	    
	    if(!isSubscriber())
		kamaroPublisher = parent->create_publisher<MessageType>(getName(), custom_qos_profile);
	    else
	    {
		using namespace std::placeholders;
		kamaroSubscription = parent->create_subscription<MessageType>(getName(), std::bind(&Entity::internalListenerCallback, this,_1), custom_qos_profile);
	    }
	    std::cout << "Created: " << getName() << " As a subscriber?: " << std::to_string(isSubscriber())<< " ptr: " << this << std::endl;
	}
	/**
	 * @brief Copy Constructor
	 */
	Entity(const Entity &t):EntityBase(t)
	{
	   
	    custom_qos_profile = t.custom_qos_profile;
	    this->parameterClient = std::make_shared<rclcpp::parameter_client::AsyncParametersClient>(parent, "ParameterServer");
	    parameterEventSubscription = parameterClient->on_parameter_event(std::bind(&Entity::onParameterEvent, this, _1));
	    listeners = t.listeners;
	   
	    
	    if(!isSubscriber())
		kamaroPublisher = parent->create_publisher<MessageType>(getName(), custom_qos_profile);
	    else
	    {
		using namespace std::placeholders;
		kamaroSubscription = parent->create_subscription<MessageType>(getName(), std::bind(&Entity::internalListenerCallback, this,_1), custom_qos_profile);
	    }
	}
	
	virtual ~Entity() { }
	
	/**
	 * @brief publish - Tell class to publish the current data to the world
	 * @return
	 */
	virtual bool publish() { }
	/**
	 * @brief add a new listener to be called when new data arrives
	 */
	void addListener(std::function<void(typename MessageType::SharedPtr)> listener) {
	    listeners.push_back(listener);
	    std::cout << "added listener to: "<< this << std::endl;
	}
	/**
	 * @brief tell the word we have new meta information (Like is a lidar mounted upside down)
	 */
	virtual void publishMetaInformation()
	{
	    std::cout << "Publish meta information in: " <<  getName() << std::endl;
	    std::vector< rclcpp::parameter::ParameterVariant> params;
	    for(auto it = internalmap.begin(); it != internalmap.end(); it++)
	    {
		params.push_back((*it)->getParameterVariant(getName()));
		(*it)->print();
	    }
	    auto set_parameters_results = this->parameterClient->set_parameters(params);
	    rclcpp::spin_until_future_complete(this->parent, set_parameters_results);
	}
	
    protected:
	/**
	 * @brief This is the method to handle new data inside your entity
	 */
	virtual void listenerCallback(const typename MessageType::SharedPtr  msg)
	{
	   // std::cout << "New message in: "<< std::endl;
	}
	
	
	
	//ROS 2 Stuff
	rmw_qos_profile_t custom_qos_profile;
	std::shared_ptr<rclcpp::parameter_client::AsyncParametersClient> parameterClient;
	std::shared_ptr<rclcpp::publisher::Publisher<MessageType>> kamaroPublisher;
	std::shared_ptr<rclcpp::subscription::Subscription<MessageType>> kamaroSubscription;
	rclcpp::subscription::Subscription<rcl_interfaces::msg::ParameterEvent>::SharedPtr parameterEventSubscription;
	
	
	
	
    private:
	std::vector<std::function<void(typename MessageType::SharedPtr)>> listeners;
	/**
	 * @brief calls the rest of the registerd listeners
	 */
	void internalListenerCallback(const typename MessageType::SharedPtr msg) {
	    std::cout << "New message in: "<< getName()<< " ptr: " <<this<< " Listeners: " << listeners.size()<<std::endl;
	    if(msg)
	    {
		listenerCallback(msg);
		for(auto listener : listeners) {
		    listener(msg);
		}
	    }
	   
	}
	/**
	 * @brief onParameterEvent
	 * @param event
	 * Gets called in case the meta information of the component has changed
	 */
	void onParameterEvent(const rcl_interfaces::msg::ParameterEvent::SharedPtr event)
	{
	    std::cout << "New parameter event in: "<<parent->get_name()  << ":" <<getName() << std::endl;
	    std::vector< rclcpp::parameter::ParameterVariant> params;
	    for (auto & new_parameter : event->new_parameters) 
	    {
		if(new_parameter.name.find(getName()) != std::string::npos)
		    params.push_back(rclcpp::parameter::ParameterVariant::from_parameter(new_parameter));
	    }
	    for (auto & changed_parameter : event->changed_parameters) 
	    {
		if(changed_parameter.name.find(getName()) != std::string::npos)
		    params.push_back(rclcpp::parameter::ParameterVariant::from_parameter(changed_parameter));
	    }
	    for (auto & parameter : params)
	    {
		std::cout << parameter.get_name() << std::endl;
		std::string reducedParameter = parameter.get_name();
		reducedParameter.erase(0,reducedParameter.find_last_of(":")+1);
		std::cout << "Reduced parameter: " << reducedParameter << std::endl;
		for (auto & internal_val : internalmap)
		{
			if(internal_val->key == reducedParameter)
			{
			    
			    
			    /*
			     * uint8 PARAMETER_NOT_SET=0
			     * uint8 PARAMETER_BOOL=1
			     * uint8 PARAMETER_INTEGER=2
			     * uint8 PARAMETER_DOUBLE=3
			     * uint8 PARAMETER_STRING=4
			     * uint8 PARAMETER_BYTES=5
			     */
			    
			    switch(parameter.get_type())
			    {
				case rcl_interfaces::msg::ParameterType::PARAMETER_BOOL:
				{
				    SpecificElement<bool>* elem = static_cast<SpecificElement<bool>*>(internal_val);
				    elem->setValue(parameter.as_bool());
				    break;
				}
				case rcl_interfaces::msg::ParameterType::PARAMETER_INTEGER:
				{
				    SpecificElement<int64_t>* elem = static_cast<SpecificElement<int64_t>*>(internal_val);
				    elem->setValue(parameter.as_int());
				    break;
				}
				case rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE:
				{
				    SpecificElement<double>* elem = static_cast<SpecificElement<double>*>(internal_val);
				    elem->setValue(parameter.as_double());
				    break;
				}
				case rcl_interfaces::msg::ParameterType::PARAMETER_STRING:
				{
				    SpecificElement<std::string>* elem = static_cast<SpecificElement<std::string>*>(internal_val);
				    elem->setValue(parameter.as_string());
				    break;
				}
				case rcl_interfaces::msg::ParameterType::PARAMETER_BYTES:
				{
				    SpecificElement<std::vector<uint8_t>>* elem = static_cast<SpecificElement<std::vector<uint8_t>>*>(internal_val);
				    elem->setValue(parameter.as_bytes());
				    break;
				}
				
			    }
			    for(auto it = internalmap.begin(); it != internalmap.end(); it++)
			    {
				(*it)->print();
			    }
			}
		}
	    }
	   
	    
	    
	}
	
    };
    
}

#endif // ENTITY_H
