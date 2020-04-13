#pragma once
#ifndef CXREQL_SERVICE_H
#define CXREQL_SERVICE_H
//#include <CoreTransport/ctReQL.h>
class CXREQL_API CXReQLService// : public Microsoft::WRL::RuntimeClass<ABI::Windows::ApplicationModel::Core::IFrameworkView>
{
	public:
		CXReQLService(ReqlService *service);
		~CXReQLService(void);


	protected:


	private:
		ReqlService * _service;
};

#endif