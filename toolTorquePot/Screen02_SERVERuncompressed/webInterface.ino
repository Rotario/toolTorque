void updateWebsocketStrings(){
  char buf[100];
  size_t len = snprintf(buf, 100, "{\"model\":\"%s\",\"chuck\":\"%s %s\",\"set\":%i}", screen.modelStr(), screen.chuckStr(), screen.shankStr(), screen.setPoint());
  webSocket.broadcastTXT(buf, len);
}

void updateWebTorque(float measured) {
  char buf[40];
  size_t len = snprintf(buf, 40, "{\"torque\":%.0f}", measured);
  webSocket.broadcastTXT(buf, len);
}
