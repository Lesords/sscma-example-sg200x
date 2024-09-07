#include <unistd.h>

#include "model.h"

namespace ma::node {

using namespace ma::engine;
using namespace ma::model;

static constexpr char TAG[] = "ma::node::model";

ModelNode::ModelNode(std::string id)
    : Node("model", id),
      uri_(""),
      debug_(true),
      count_(0),
      engine_(nullptr),
      model_(nullptr),
      thread_(nullptr),
      camera_(nullptr),
      raw_frame_(1),
      jpeg_frame_(1) {}

ModelNode::~ModelNode() {

    if (thread_ != nullptr) {
        delete thread_;
    }

    if (camera_ != nullptr) {
        camera_->detach(CHN_RAW, &raw_frame_);
        if (debug_) {
            camera_->detach(CHN_JPEG, &jpeg_frame_);
        }
        delete camera_;
    }

    if (model_ != nullptr) {
        delete model_;
    }
    if (engine_ != nullptr) {
        delete engine_;
    }
}


void ModelNode::threadEntry() {


    ma_err_t err           = MA_OK;
    const ma_img_t* img    = nullptr;
    Detector* detector     = nullptr;
    Classifier* classifier = nullptr;
    videoFrame* raw        = nullptr;
    videoFrame* jpeg       = nullptr;

    switch (model_->getType()) {
        case MA_MODEL_TYPE_FOMO:
        case MA_MODEL_TYPE_YOLOV8:
        case MA_MODEL_TYPE_YOLO_WORLD:
        case MA_MODEL_TYPE_NVIDIA_DET:
        case MA_MODEL_TYPE_YOLOV5:
            detector = static_cast<Detector*>(model_);
            img      = detector->getInputImg();
            break;
        case MA_MODEL_TYPE_IMCLS:
            classifier = static_cast<Classifier*>(model_);
            img        = classifier->getInputImg();
            break;
        default:
            break;
    }

    // wait for dependencies ready
    while (true) {
        for (auto dep : dependencies_) {
            auto n = NodeFactory::find(dep);
            if (n == nullptr) {
                break;
            }
            if (camera_ == nullptr && n->type() == "camera") {
                Thread::enterCritical();
                camera_ = reinterpret_cast<CameraNode*>(n);
                camera_->config(CHN_RAW, img->width, img->height, 30, img->format);
                camera_->attach(CHN_RAW, &raw_frame_);
                if (debug_) {
                    camera_->config(CHN_JPEG, img->width, img->height, 30, MA_PIXEL_FORMAT_JPEG);
                    camera_->attach(CHN_JPEG, &jpeg_frame_);
                }
                camera_->onStart();
                Thread::exitCritical();
                break;
            }
        }
        if (camera_ != nullptr) {
            break;
        }
        Thread::sleep(Tick::fromMilliseconds(10));
    }


    while (true) {
        if (!raw_frame_.fetch(reinterpret_cast<void**>(&raw))) {
            continue;
        }
        if (debug_ && !jpeg_frame_.fetch(reinterpret_cast<void**>(&jpeg))) {
            raw->release();
            continue;
        }
        Thread::enterCritical();
        json reply = json::object({{"type", MA_MSG_TYPE_EVT},
                                   {"name", "invoke"},
                                   {"code", MA_OK},
                                   {"data", {{"count", ++count_}}}});

        reply["data"]["resolution"] = json::array({raw->img.width, raw->img.height});

        ma_tensor_t tensor = {
            .is_physical = true,
            .is_variable = false,
        };
        tensor.data.addr = raw->phy_addr;
        engine_->setInput(0, tensor);
        model_->setRunDone([this, raw](void* ctx) { raw->release(); });
        if (detector != nullptr) {
            err           = detector->run(nullptr);
            auto _perf    = detector->getPerf();
            auto _results = detector->getResults();
            reply["data"]["perf"].push_back({_perf.preprocess, _perf.inference, _perf.postprocess});
            reply["data"]["boxes"] = json::array();
            for (auto& result : _results) {
                reply["data"]["boxes"].push_back({static_cast<int16_t>(result.x * raw->img.width),
                                                  static_cast<int16_t>(result.y * raw->img.height),
                                                  static_cast<int16_t>(result.w * raw->img.width),
                                                  static_cast<int16_t>(result.h * raw->img.height),
                                                  static_cast<int8_t>(result.score * 100),
                                                  result.target});
            }
        } else if (classifier != nullptr) {
            err           = classifier->run(nullptr);
            auto _perf    = classifier->getPerf();
            auto _results = classifier->getResults();
            reply["data"]["perf"].push_back({_perf.preprocess, _perf.inference, _perf.postprocess});
            reply["data"]["classes"] = json::array();
            for (auto& result : _results) {
                reply["data"]["classes"].push_back(
                    {static_cast<int8_t>(result.score * 100), result.target});
            }
        }


        if (debug_) {
            int base64_len = 4 * ((jpeg->img.size + 2) / 3 + 2);
            char* base64   = new char[base64_len];
            if (base64 != nullptr) {
                memset(base64, 0, base64_len);
                ma::utils::base64_encode(jpeg->img.data, jpeg->img.size, base64, &base64_len);
                reply["data"]["image"] = std::string(base64, base64_len);
                jpeg->release();
                delete[] base64;
            }
        } else {
            reply["data"]["image"] = "";
        }

        server_->response(id_, reply);

        Thread::exitCritical();
    }
}

void ModelNode::threadEntryStub(void* obj) {
    reinterpret_cast<ModelNode*>(obj)->threadEntry();
}

ma_err_t ModelNode::onCreate(const json& config) {
    ma_err_t err = MA_OK;

    Guard guard(mutex_);

    if (!config.contains("uri")) {
        throw NodeException(MA_EINVAL, "uri not found");
    }

    uri_ = config["uri"].get<std::string>();


    if (uri_.empty()) {
        throw NodeException(MA_EINVAL, "uri is empty");
    }

    if (access(uri_.c_str(), R_OK) != 0) {
        throw NodeException(MA_EINVAL, "model not found: " + uri_);
    }

    try {
        engine_ = new EngineDefault();

        if (engine_ == nullptr) {
            throw NodeException(MA_ENOMEM, "Engine create failed");
        }
        if (engine_->init() != MA_OK) {
            throw NodeException(MA_EINVAL, "Engine init failed");
        }
        if (engine_->load(uri_) != MA_OK) {
            throw NodeException(MA_EINVAL, "Model load failed");
        }
        model_ = ModelFactory::create(engine_);
        if (model_ == nullptr) {
            throw NodeException(MA_EINVAL, "Model create failed");
        }

        MA_LOGI(TAG, "model: %s %s", uri_.c_str(), model_->getName());
        {  // extra config
            if (config.contains("tscore")) {
                model_->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, config["tscore"].get<float>());
            }
            if (config.contains("tiou")) {
                model_->setConfig(MA_MODEL_CFG_OPT_NMS, config["tiou"].get<float>());
            }
            if (config.contains("topk")) {
                model_->setConfig(MA_MODEL_CFG_OPT_TOPK, config["tiou"].get<float>());
            }
            if (config.contains("debug")) {
                debug_ = config["debug"].get<bool>();
            }
            if (config.contains("trace")) {
                trace_ = config["trace"].get<bool>();
            }
        }
        thread_ = new Thread((type_ + "#" + id_).c_str(), &ModelNode::threadEntryStub);
        if (thread_ == nullptr) {
            throw NodeException(MA_ENOMEM, "thread create failed");
        }

    } catch (std::exception& e) {
        if (engine_ != nullptr) {
            delete engine_;
            engine_ = nullptr;
        }
        if (model_ != nullptr) {
            delete model_;
            model_ = nullptr;
        }
        if (thread_ != nullptr) {
            delete thread_;
            thread_ = nullptr;
        }
        throw NodeException(err, e.what());
    }

    server_->response(
        id_,
        json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", MA_OK}, {"data", ""}}));
    return MA_OK;

    return MA_OK;
}


ma_err_t ModelNode::onMessage(const json& msg) {
    Guard guard(mutex_);
    ma_err_t err    = MA_OK;
    std::string cmd = msg["name"].get<std::string>();
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    if (cmd == "config") {
        if (msg["data"].contains("tscore")) {
            model_->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, msg["data"]["tscore"].get<float>());
        }
        if (msg["data"].contains("tiou")) {
            model_->setConfig(MA_MODEL_CFG_OPT_NMS, msg["data"]["tiou"].get<float>());
        }
        if (msg["data"].contains("topk")) {
            model_->setConfig(MA_MODEL_CFG_OPT_TOPK, msg["data"]["tiou"].get<float>());
        }
        if (msg["data"].contains("debug")) {
            debug_ = msg["data"]["debug"].get<bool>();
        }
        if (msg["data"].contains("trace")) {
            trace_ = msg["data"]["trace"].get<bool>();
        }
        server_->response(id_,
                          json::object({{"type", MA_MSG_TYPE_RESP},
                                        {"name", cmd},
                                        {"code", MA_OK},
                                        {"data", msg["data"]}}));
    } else {
        throw NodeException(MA_EINVAL, "invalid cmd: " + cmd);
    }
    return MA_OK;
}

ma_err_t ModelNode::onStart() {
    Guard guard(mutex_);
    if (started_) {
        return MA_OK;
    }
    started_ = true;
    count_   = 0;
    thread_->start(this);
    return MA_OK;
}

ma_err_t ModelNode::onStop() {
    Guard guard(mutex_);
    if (!started_) {
        return MA_OK;
    }
    started_ = false;
    if (thread_ != nullptr) {
        thread_->stop();
    }
    if (camera_ != nullptr) {
        camera_->detach(CHN_RAW, &raw_frame_);
        if (debug_) {
            camera_->detach(CHN_JPEG, &jpeg_frame_);
        }
        camera_ = nullptr;
    }
    return MA_OK;
}

ma_err_t ModelNode::onDestroy() {

    Guard guard(mutex_);

    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }
    if (engine_ != nullptr) {
        delete engine_;
        engine_ = nullptr;
    }
    if (model_ != nullptr) {
        delete model_;
        model_ = nullptr;
    }
    if (camera_ != nullptr) {
        camera_->detach(CHN_RAW, &raw_frame_);
        if (debug_) {
            camera_->detach(CHN_JPEG, &jpeg_frame_);
        }
    }

    server_->response(
        id_,
        json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "destroy"}, {"code", MA_OK}, {"data", ""}}));
    return MA_OK;
}

REGISTER_NODE_SINGLETON("model", ModelNode);

}  // namespace ma::node