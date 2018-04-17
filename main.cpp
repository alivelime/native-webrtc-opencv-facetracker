#include "webrtc.h"

#include "session.h"
#include "server.h"
#include "customvideocapturer.h"
#include "videorenderer.h"

struct ConnectionData {
  websocketpp::connection_hdl hdl;
  std::unique_ptr<VideoRenderer> renderer;
  std::unique_ptr<PeerConnectionCallback> callback;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;
  Session session;

   ConnectionData(
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> p,
    PeerConnectionCallback* c,
    websocketpp::connection_hdl h
  ) : hdl(h), callback(c), pc(p) {}

  ~ConnectionData() {
    // do not change the order.!
    renderer.reset();
    pc = NULL;
    // Do not change the order.
    
    LOG(INFO) << "~ConnectionData()";
  }
};

int main() {

  rtc::InitializeSSL();

  // mapping between socket connection and peer connection.
  std::unordered_map<void*, std::unique_ptr<ConnectionData>> connections;

	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory
    = webrtc::CreatePeerConnectionFactory();

  WebsocketServer *ws = ServerInit(
    // on http = on connection
    [](WebsocketServer* s, websocketpp::connection_hdl hdl) {
        WebsocketServer::connection_ptr con = s->get_con_from_hdl(hdl);
        
        con->set_body("Hello World!");
        con->set_status(websocketpp::http::status_code::ok);
    },
    [&](WebsocketServer* s, websocketpp::connection_hdl hdl) {
      LOG(INFO) << "on_open ";

      // create and set peer connection.
      auto callback = new PeerConnectionCallback();
      connections[hdl.lock().get()] = std::unique_ptr<ConnectionData> (
        new ConnectionData(
          CreatePeerConnection(callback),
          callback,
          hdl
        )
      );
      auto& connection = connections[hdl.lock().get()];
      auto& peer_connection = connection->pc;
      auto& peer_connection_callback = connection->callback;

      peer_connection_callback->SetOnAddStream(
        [&](rtc::scoped_refptr<webrtc::MediaStreamInterface> stream){
          webrtc::VideoTrackVector tracks = stream->GetVideoTracks();

          std::unique_ptr<CustomVideoCapturer> capturer(new CustomVideoCapturer(connection->session));
          connection->renderer.reset(new VideoRenderer(1, 1, tracks[0], *capturer ));

          rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
              peer_connection_factory->CreateVideoTrack(
                  "video_label",
                  peer_connection_factory->CreateVideoSource(
                    std::move(capturer),  NULL)
              )
          );

          rtc::scoped_refptr<webrtc::MediaStreamInterface> new_stream =
              peer_connection_factory->CreateLocalMediaStream("stream_label");

          for (auto &track : stream->GetAudioTracks()) {new_stream->AddTrack(track);}
          new_stream->AddTrack(video_track);
          if (!peer_connection->AddStream(new_stream)) {
            LOG(LS_ERROR) << "Adding stream to PeerConnection failed";
          }
      });
      peer_connection_callback->SetOnIceCandidate([&](const webrtc::IceCandidateInterface* candidate){
        std::string sdp;
        candidate->ToString(&sdp);
        LOG(INFO) << "send ice candidate... " << sdp;

        Json::StyledWriter writer;
        Json::Value jmessage;

        jmessage["candidate"]["sdpMid"] = candidate->sdp_mid();
        jmessage["candidate"]["sdpMLineIndex"] = candidate->sdp_mline_index();
        jmessage["candidate"]["candidate"] = sdp;

        try {
          s->send(connection->hdl, writer.write(jmessage), websocketpp::frame::opcode::text);
        } catch (const websocketpp::lib::error_code& e) {
            LOG(LERROR) << "Echo failed because: " << e
                        << "(" << e.message() << ")";
        }

        LOG(INFO) << "send ice candidate end. ";
      });
    },
    [&](WebsocketServer* s, websocketpp::connection_hdl hdl) {
      LOG(INFO) << "on_close ";

      connections.erase(hdl.lock().get());
    },
    // on message
    [&](WebsocketServer* s, websocketpp::connection_hdl hdl, message_ptr msg) {
      LOG(INFO) << "on_message called with hdl: " << hdl.lock().get()
                << " and message: " << msg->get_payload()
                << std::endl;

      auto& connection = connections[hdl.lock().get()];
      connection->hdl = hdl;
      auto& peer_connection = connection->pc;

      // receive as JSON.
      Json::Reader reader;
      Json::Value request;
      if (!reader.parse(msg->get_payload(), request)) {
        LOG(WARNING) << "Received unknown message. " << msg->get_payload();
        return;
      }

      // receive offer
      if (request["type"] == "offer") {
        LOG(INFO) << "1 -------------------- receive client offer ---------------------";

        std::string sdp = request["sdp"].asString();
        webrtc::SdpParseError error;
        webrtc::SessionDescriptionInterface* session_description(webrtc::CreateSessionDescription("offer", sdp, &error));
        if (!session_description) {
          LOG(WARNING) << "Can't parse received session description message. "
            << "SdpParseError was: " << error.description;
          return;
        }


        LOG(INFO) << " set remote offer description start.";
        peer_connection->SetRemoteDescription( DummySetSessionDescriptionObserver::Create(), session_description);
        LOG(INFO) << " set remote offer description end.";
        
        // send answer
        LOG(INFO) << " create answer start.";
        peer_connection->CreateAnswer(new rtc::RefCountedObject<CreateSDPCallback>(
          [&](webrtc::SessionDescriptionInterface* desc) {
            LOG(INFO) << " create answer callback start.";
            peer_connection->SetLocalDescription( DummySetSessionDescriptionObserver::Create(), desc);

            std::string sdp;
            desc->ToString(&sdp);

            Json::StyledWriter writer;
            Json::Value jmessage;
            jmessage["type"] = desc->type();
            jmessage["sdp"] = sdp;

            LOG(INFO) << "2 -------------------- send server answer ---------------------";
            try {
              LOG(INFO) << " sending answer..." << writer.write(jmessage);
              s->send(hdl, writer.write(jmessage), msg->get_opcode());
            } catch (const websocketpp::lib::error_code& e) {
                LOG(LERROR) << "Echo failed because: " << e
                            << "(" << e.message() << ")";
            }
            LOG(INFO) << " create answer callback end.";

          },
          nullptr
        ), NULL);

        // --- type == "offer" ---
      } else if (request["type"] == "join") {

        // create offer
        LOG(INFO) << " create offer start.";

        peer_connection->CreateOffer(new rtc::RefCountedObject<CreateSDPCallback> (
          [&](webrtc::SessionDescriptionInterface* desc){
            LOG(INFO) << " create offer callback start.";
            peer_connection->SetLocalDescription( DummySetSessionDescriptionObserver::Create(), desc);

            // send SDP. trickle ICE.
            std::string sdp;
            desc->ToString(&sdp);

            Json::StyledWriter writer;
            Json::Value jmessage;
            jmessage["type"] = desc->type();
            jmessage["sdp"] = sdp;
            
            LOG(INFO) << "3 -------------------- send server offer ---------------------";
            try {
              LOG(INFO) << " sending offer..." << writer.write(jmessage);
              s->send(hdl, writer.write(jmessage), msg->get_opcode());
            } catch (const websocketpp::lib::error_code& e) {
                LOG(LERROR) << "Echo failed because: " << e
                            << "(" << e.message() << ")";
            }

            LOG(INFO) << " create offer callback start.";
          },
          nullptr
        ), NULL);

      } else if (request["type"] == "answer") {
        // receive answer
        LOG(INFO) << "4 -------------------- receive client answer ---------------------";
        std::string sdp = request["sdp"].asString();

        webrtc::SdpParseError error;
        webrtc::SessionDescriptionInterface* session_description(webrtc::CreateSessionDescription("answer", sdp, &error));
        if (!session_description) {
          LOG(WARNING) << "Can't parse received session description message. "
            << "SdpParseError was: " << error.description;
          return;
        }
        LOG(INFO) << " set remote answer description start.";
        peer_connection->SetRemoteDescription( DummySetSessionDescriptionObserver::Create(), session_description);
        LOG(INFO) << " set remote answer description end.";
      } else if (request.isMember("candidate")) {
        LOG(INFO) << "on ice candidate.";

        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::IceCandidateInterface> candidate(
            webrtc::CreateIceCandidate(
              request["candidate"]["sdpMid"].asString(),
              request["candidate"]["sdpMLineIndex"].asInt(),
              request["candidate"]["candidate"].asString(),
              &error)
        );
        if (!candidate.get()) {
          LOG(WARNING) << "Can't parse received candidate message. "
              << "SdpParseError was: " << error.description;
          return;
        }
        if (!peer_connection->AddIceCandidate(candidate.get())) {
          LOG(WARNING) << "Failed to apply the received candidate";
          return;
        }
        LOG(INFO) << "set ice  candidate end.";
      } else if (request.isMember("mode")) {
        connection->session.mode.onAir = request["mode"]["onAir"].asBool();
        connection->session.mode.isKaicho = request["mode"]["isKaicho"].asBool();
        connection->session.mode.useEye = request["mode"]["useEye"].asBool();
        connection->session.mode.useMustache = request["mode"]["useMustache"].asBool();
      }

      ProcessMessage();
      LOG(INFO) << "end of request.";
    } // on_message
  );
	
  LOG(INFO) << "run server.";
  ws->run();
  rtc::CleanupSSL();

	return 0;
};

