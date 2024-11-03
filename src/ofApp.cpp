#include "ofApp.h"
#include "ofxTimeMeasurements.h"
#include "dkm.hpp"

//--------------------------------------------------------------
void ofApp::setup(){
  ofSetFrameRate(Constants::FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(Constants::FRAME_RATE);
  
  audioParameters.add(validLowerRmsParameter);
  audioParameters.add(validLowerPitchParameter);
  audioParameters.add(validUpperPitchParameter);
  audioParameters.add(minPitchParameter);
  audioParameters.add(maxPitchParameter);
  audioParameters.add(minRMSParameter);
  audioParameters.add(maxRMSParameter);
  audioParameters.add(minSpectralKurtosisParameter);
  audioParameters.add(maxSpectralKurtosisParameter);
  audioParameters.add(minSpectralCentroidParameter);
  audioParameters.add(maxSpectralCentroidParameter);
  parameters.add(audioParameters);

  clusterParameters.add(clusterCentresParameter);
  clusterParameters.add(clusterSourceSamplesMaxParameter);
  clusterParameters.add(clusterDecayRateParameter);
  clusterParameters.add(sameClusterToleranceParameter);
  clusterParameters.add(sampleNoteClustersParameter);
  clusterParameters.add(sampleNotesParameter);
  parameters.add(clusterParameters);
  
  gui.setup(parameters);

  ofxTimeMeasurements::instance()->setEnabled(false);
}

//--------------------------------------------------------------
void ofApp::update(){
  TS_START("update-introspection");
  introspector.update();
  TS_STOP("update-introspection");

  TS_START("update-audoanalysis");
  audioDataProcessorPtr->update();
  TS_STOP("update-audoanalysis");

  float s = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch, minPitchParameter, maxPitchParameter);// 700.0, 1300.0);
  float t = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare, minRMSParameter, maxRMSParameter); ////400.0, 4000.0, false);
  float u = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::spectralKurtosis, minSpectralKurtosisParameter, maxSpectralKurtosisParameter);
  float v = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid, minSpectralCentroidParameter, maxSpectralCentroidParameter);
  
  std::vector<ofxAudioData::ValiditySpec> sampleValiditySpecs {
    {ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare, false, validLowerRmsParameter},
    {ofxAudioAnalysisClient::AnalysisScalar::pitch, false, validLowerPitchParameter},
    {ofxAudioAnalysisClient::AnalysisScalar::pitch, true, validUpperPitchParameter}
  };

  if (audioDataProcessorPtr->isDataValid(sampleValiditySpecs)) {
    
    TS_START("maintain-recent-notes");
    if (recentNoteXYs.size() > clusterSourceSamplesMaxParameter) {
      recentNoteXYs.erase(recentNoteXYs.end() - clusterSourceSamplesMaxParameter/10, recentNoteXYs.end());
    }
    recentNoteXYs.push_back({ s, t });
    introspector.addCircle(s, t, 1.0/Constants::WINDOW_WIDTH*5.0, ofColor::yellow, true, 30); // introspection: small yellow circle for new raw source sample
    TS_STOP("maintain-recent-notes");

    std::tuple<std::vector<std::array<float, 2>>, std::vector<uint32_t>> clusterResults;
    TS_START("update-kmeans");
    if (recentNoteXYs.size() > clusterCentresParameter) {
      dkm::clustering_parameters<float> params(clusterCentresParameter);
      params.set_random_seed(1000); // keep clusters stable
      clusterResults = dkm::kmeans_lloyd(recentNoteXYs, params);
    }
    TS_STOP("update-kmeans");
    
    TS_START("update-clusterCentres");
    {
      // glm::vec4 w is age
      // add to clusterCentres from new clusters
      for (const auto& cluster : std::get<0>(clusterResults)) {
        float x = cluster[0]; float y = cluster[1];
        auto it = std::find_if(clusterCentres.begin(),
                               clusterCentres.end(),
                               [x, y, this](const glm::vec4& p) {
          return ((std::abs(p.x-x) < sameClusterToleranceParameter) && (std::abs(p.y-y) < sameClusterToleranceParameter));
        });
        if (it == clusterCentres.end()) {
          // don't have this clusterCentre so make it
          clusterCentres.push_back({ x, y, 0.0, 1.0 }); // start at age=1
          introspector.addCircle(x, y, 15.0*1.0/Constants::WINDOW_WIDTH, ofColor::red, true, 10); // introspection: small red circle is new cluster centre
        } else {
          // existing cluster so increase its age to preserve it
          it->w++;
          introspector.addCircle(it->x, it->y, 3.0*1.0/Constants::WINDOW_WIDTH, ofColor::red, true, 25); // introspection: large red circle is existing cluster centre that continues to exist
        }
      }
    }
    TS_STOP("update-clusterCentres");
  }
  
  TS_START("decay-clusterCentres");
  for (auto& p: clusterCentres) {
    p.w *= clusterDecayRateParameter;
  }
  // delete decayed clusterCentres
  clusterCentres.erase(std::remove_if(clusterCentres.begin(),
                                      clusterCentres.end(),
                                      [](const glm::vec4& n) { return n.w <= 1.0; }),
                       clusterCentres.end());
  TS_STOP("decay-clusterCentres");
  
  TS_START("update-divider");
  bool _ = dividedArea.updateUnconstrainedDividerLines(clusterCentres, {(size_t)ofRandom(clusterCentres.size()), (size_t)ofRandom(clusterCentres.size())});
  if (recentNoteXYs.size() > 2) {
    dividedArea.clearConstrainedDividerLines();
    for (auto iter = recentNoteXYs.cbegin(); iter != recentNoteXYs.cend() - 1; iter++) {
      auto p1 = *iter; auto p2 = *(iter + 1);
      dividedArea.addConstrainedDividerLine({p1[0], p1[1]}, {p2[0], p2[1]});
    }
  }
  TS_STOP("update-divider");
  
  TS_START("update-plottable");
  {
    for(auto& l : dividedArea.unconstrainedDividerLines) {
      plot.addLine(l.start.x, l.start.y, l.end.x, l.end.y, ofColor::black, 2);
    }
    for(auto& l : dividedArea.constrainedDividerLines) {
      plot.addLine(l.start.x, l.start.y, l.end.x, l.end.y, ofColor::grey, 2);
    }
    for (auto& p: clusterCentres) {
      if (p.w < 10.0) continue;
      float radius = std::fmod(p.w*1.0/100.0, 1.0/15.0);
      plot.addArc(p.x, p.y, radius, -180.0*(u+p.x), 180.0*(v+p.y), ofColor::blue, 15);
    }
  }
  plot.update();
  TS_STOP("update-plottable");
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofClear(255.0, 255.0);

  plot.draw();
  
  // introspection
  {
    TS_START("draw-introspection");
    ofPushStyle();
    ofPushView();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofScale(Constants::WINDOW_WIDTH); // drawing on Introspection is normalised so scale up
    introspector.draw();
    ofPopView();
    ofPopStyle();
    TS_STOP("draw-introspection");
  }
  
  // audio analysis graphs
  {
    ofPushStyle();
    ofPushView();
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    float plotHeight = ofGetWindowHeight() / 4.0;
    audioDataPlotsPtr->drawPlots(ofGetWindowWidth(), plotHeight);
    audioDataSpectrumPlotsPtr->draw();
    ofPopView();
    ofPopStyle();
  }

  // gui
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (audioAnalysisClientPtr->keyPressed(key)) return;
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
  {
    float plotHeight = ofGetWindowHeight() / 4.0;
    int plotIndex = ofGetMouseY() / plotHeight;
    bool plotKeyPressed = audioDataPlotsPtr->keyPressed(key, plotIndex);
    bool spectrumPlotKeyPressed = audioDataSpectrumPlotsPtr->keyPressed(key);
    if (plotKeyPressed || spectrumPlotKeyPressed) return;
  }
  if (introspector.keyPressed(key)) return;
  if (plot.keyPressed(key)) return;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
