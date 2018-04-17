let serverUrl = 'aisatsu.tk:9001';
let socket;

window.RTCPeerConnection = window.RTCPeerConnection ||
                           window.webkitRTCPeerConnection ||
                           window.mozRTCPeerConnection;
window.RTCSessionDescription = window.RTCSessionDescription ||
                               window.mozRTCSessionDescription;
window.RTCIceCandidate = window.RTCIceCandidate || 
                         window.mozRTCIceCandidate;

let pc;

window.onload = (e) => {
  socket = new WebSocket('wss://' + serverUrl);
  socket.onopen = function () {
    console.log('socket open');
    pc = new RTCPeerConnection({ "iceServers": [
    {urls:'stun:35.189.141.152:3478'}, {urls:'turn:35.189.141.152:3478?transport=udp', username:'aisatsu', credential:'aisatsu'}
      ]}
    );

    // send any ice candidates to the other peer
    pc.onicecandidate = function (evt) {
      if (evt.candidate) {
        socket.send(JSON.stringify({ "candidate": evt.candidate }));
      }

    };

    // let the "negotiationneeded" event trigger offer generation
    pc.onnegotiationneeded = function () {
      console.log('on negotiation needed!');
      createOffer();
    }

    // once remote stream arrives, show it in the remote video element
    pc.onaddstream = function (evt) {
      remoteView.srcObject = evt.stream;
    };
    pc.ontrack = function (evt) {
				console.log("ontrack.");
        if (evt.track.kind === "video")
          remoteView.srcObject = evt.streams[0];
    };

    // get a local stream, show it in a self-view and add it to be sent
    navigator.mediaDevices.getUserMedia({ "audio": false, "video": { width: 640, height: 480 } })
    .then(function (stream) {
      selfView.srcObject = stream;
      if (typeof pc.addTrack === "function") {
        pc.addTrack(stream.getVideoTracks()[0], stream);
      } else {
        pc.addStream(stream);
      }

      // createOffer();
      changeStatus("&#x25B6;");
      changeButton("&#x25B6;");
    })
    .catch(logError);
  };
  socket.onmessage = function (evt) {
    console.log('on message from server.');
    let message = JSON.parse(evt.data);
    console.log(message);

    if (message.type === 'answer') {
      pc.setRemoteDescription(new RTCSessionDescription(message),() => {
        socket.send(JSON.stringify({type: 'join'}));
      }, logError);
    } else if (message.type === 'offer') {
      pc.setRemoteDescription(new RTCSessionDescription(message), ()=>{
        createAnswer();
      }, logError);
    } else {
      // ICE candidate.
      pc.addIceCandidate(new RTCIceCandidate(message.candidate), ()=>{}, logError);
    }
  };
  socket.onclose = function () {
      console.log('closed socket by server.');
  };
  socket.onerror = function (e) {
    document.getElementById('status').innerText = 'websocket error.';
    console.log(e);
  };
};


function createOffer() {
  pc.createOffer()
  .then((desc) => {
    console.log(desc);
    pc.setLocalDescription(desc);
    socket.send(JSON.stringify(desc));
  })
  .catch(logError);
}

function createAnswer(desc) {
  pc.createAnswer()
  .then((desc) => {
    console.log(desc);
    pc.setLocalDescription(desc);
    socket.send(JSON.stringify(desc));
  })
  .catch(logError);
}

function logError(error) {
  console.log(error.name + ": " + error.message);
}

let mode = {
  onAir: false,
  isKaicho: false,
  useEye: false,
  useMsutache: false,
};
function switchMode(target) {
  mode[target] = !mode[target]; 
  socket.send(JSON.stringify({"mode": mode}));
}
function changeStatus(text) {
  document.getElementById('status').innerHTML = text;
}
function switchStart() {
  let text, target = 'onAir';
  switchMode(target);
  changeStatus('');
}
