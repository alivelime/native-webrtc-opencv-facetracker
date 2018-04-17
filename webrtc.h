#ifndef OMEKASHI_WEBRTC_H
#define OMEKASHI_WEBRTC_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/base/json.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/ssladapter.h"

class DummySetSessionDescriptionObserver
: public webrtc::SetSessionDescriptionObserver {
public:
   static DummySetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		LOG(INFO) << __FUNCTION__;
	}
	virtual void OnFailure(const std::string& error) {
		LOG(INFO) << __FUNCTION__ << " " << error;
	}

	protected:
	DummySetSessionDescriptionObserver() {}
	~DummySetSessionDescriptionObserver() {}
};

class PeerConnectionCallback : public webrtc::PeerConnectionObserver{
	private:
		std::function<void(rtc::scoped_refptr<webrtc::MediaStreamInterface>)> onAddStream;
		std::function<void(const webrtc::IceCandidateInterface*)> onIceCandidate;
	public:
		PeerConnectionCallback() 
		: onAddStream(nullptr), onIceCandidate(nullptr) {
		}
		virtual ~PeerConnectionCallback() {}
    void SetOnAddStream(std::function<void(rtc::scoped_refptr<webrtc::MediaStreamInterface>)> callback) {
      onAddStream = callback;
    }
    void SetOnIceCandidate(std::function<void(const webrtc::IceCandidateInterface*)> callback) {
      onIceCandidate = callback;
    }
protected:
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override{
	  LOG(INFO) << __FUNCTION__ << " " << new_state;
  }
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {
	  LOG(INFO) << __FUNCTION__ << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1";
  }
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override{
	  LOG(INFO) << __FUNCTION__ << " " << new_state;
  };
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override{
	  LOG(INFO) << __FUNCTION__ << " " << new_state;
  };
  void OnIceConnectionReceivingChange(bool receiving) override {
	  LOG(INFO) << __FUNCTION__ << " " << receiving;
  }

  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
    onIceCandidate(candidate);
  };
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
	  LOG(INFO) << __FUNCTION__ << " " << stream->label();
  };
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
    LOG(INFO) << __FUNCTION__ << " " << stream->label();
    onAddStream(stream);
  };

};

class CreateSDPCallback : public webrtc::CreateSessionDescriptionObserver {
	private:
		std::function<void(webrtc::SessionDescriptionInterface*)> success;
		std::function<void(const std::string&)> failure;
	public:
		CreateSDPCallback(std::function<void(webrtc::SessionDescriptionInterface*)> s, std::function<void(const std::string&)> f)
      : success(s), failure(f) {
		};
		void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
      LOG(INFO) << __FUNCTION__ ;
			if (success) {
				success(desc);
			}
		}
		void OnFailure(const std::string& error) {
      LOG(INFO) << __FUNCTION__ ;
			if (failure) {
				failure(error);
			} else {
				LOG(LERROR) << error;
			}
		}
};

rtc::scoped_refptr<webrtc::PeerConnectionInterface> CreatePeerConnection(webrtc::PeerConnectionObserver* observer) {
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory
    = webrtc::CreatePeerConnectionFactory();
	if (!peer_connection_factory.get()) {
		LOG(LERROR) << "Failed to initialize PeerConnectionFactory" << std::endl;
	  return nullptr;
	}

  webrtc::FakeConstraints constraints;
  constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp, "true");

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::PeerConnectionInterface::IceServer server;
  
	server.uri = "stun:52.68.211.241:3478";
	config.servers.push_back(server);

  return peer_connection_factory->CreatePeerConnection(
      config, &constraints, NULL, NULL, observer);
}

void ProcessMessage() {
  auto thread = rtc::Thread::Current();
  auto msg_cnt = thread->size();
  LOG(INFO) << "process message. : last " << msg_cnt;

  while(msg_cnt > 0) {
    rtc::Message msg;
    if (!thread->Get(&msg, 0))
      return;
    thread->Dispatch(&msg);
  }
}


#endif // OMEKASHI_WEBRTC_H
