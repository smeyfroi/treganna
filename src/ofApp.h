#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxAudioAnalysisClient.h"
#include "ofxAudioData.h"
#include "ofxIntrospector.h"
#include "ofxPlottable.h"
#include "Constants.h"
#include "ofxDividedArea.h"

class ofApp : public ofBaseApp{

public:
  void setup() override;
  void update() override;
  void draw() override;
  void exit() override;
  
  void keyPressed(int key) override;
  void keyReleased(int key) override;
  void mouseMoved(int x, int y ) override;
  void mouseDragged(int x, int y, int button) override;
  void mousePressed(int x, int y, int button) override;
  void mouseReleased(int x, int y, int button) override;
  void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
  void mouseEntered(int x, int y) override;
  void mouseExited(int x, int y) override;
  void windowResized(int w, int h) override;
  void dragEvent(ofDragInfo dragInfo) override;
  void gotMessage(ofMessage msg) override;
  
private:
  void updateRecentNotes(float s, float t, float u, float v);
  void updateClusters();

  std::shared_ptr<ofxAudioAnalysisClient::FileClient> audioAnalysisClientPtr;
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;
  std::shared_ptr<ofxAudioData::Plots> audioDataPlotsPtr;
  std::shared_ptr<ofxAudioData::SpectrumPlots> audioDataSpectrumPlotsPtr;

  DividedArea dividedArea { {1.0, 1.0}, 7 };

  std::vector<std::array<float, 2>> recentNoteXYs;
  std::vector<glm::vec4> clusterCentres;
  
  Plottable plot { Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT, true }; // We draw in normalised coords so scale up for drawing and saving into a window-shaped viewport
  
  Introspector introspector;
  
  bool guiVisible { false };
  ofxPanel gui;
  ofParameterGroup parameters;
  
  ofParameterGroup audioParameters { "audio" };
  ofParameter<float> validLowerRmsParameter { "validLowerRms", 300.0, 100.0, 5000.0 };
  ofParameter<float> validLowerPitchParameter { "validLowerPitch", 50.0, 50.0, 8000.0 };
  ofParameter<float> validUpperPitchParameter { "validUpperPitch", 5000.0, 50.0, 8000.0 };
  ofParameter<float> minPitchParameter { "minPitch", 200.0, 0.0, 8000.0 };
  ofParameter<float> maxPitchParameter { "maxPitch", 1800.0, 0.0, 8000.0 };
  ofParameter<float> minRMSParameter { "minRMS", 0.0, 0.0, 6000.0 };
  ofParameter<float> maxRMSParameter { "maxRMS", 4600.0, 0.0, 6000.0 };
  ofParameter<float> minSpectralKurtosisParameter { "minSpectralKurtosis", 0.0, 0.0, 6000.0 };
  ofParameter<float> maxSpectralKurtosisParameter { "maxSpectralKurtosis", 25.0, 0.0, 6000.0 };
  ofParameter<float> minSpectralCentroidParameter { "minCentroidKurtosis", 0.4, 0.0, 10.0 };
  ofParameter<float> maxSpectralCentroidParameter { "maxCentroidKurtosis", 6.0, 0.0, 10.0 };

  ofParameterGroup clusterParameters { "cluster" };
  ofParameter<int> clusterCentresParameter { "clusterCentres", 12, 2.0, 50.0 };
  ofParameter<int> clusterSourceSamplesMaxParameter { "clusterSourceSamplesMax", 3000, 1000, 8000 }; // Note: 1600 raw samples per frame at 30fps
  ofParameter<float> clusterDecayRateParameter { "clusterDecayRate", 0.94, 0.0, 1.0 };
  ofParameter<float> sameClusterToleranceParameter { "sameClusterTolerance", 0.1, 0.01, 1.0 };
  ofParameter<int> sampleNoteClustersParameter { "sampleNoteClusters", 7, 1, 20 };
  ofParameter<int> sampleNotesParameter { "sampleNotes", 7, 1, 20 };

};
