#include "layer.hpp"
#include <algorithm>

// #@@range_begin(layer_ctor)
Layer::Layer(unsigned int id) : id_{id} {
}
// #@@range_end(layer_ctor)

// #@@range_begin(layer_id)
unsigned int Layer::ID() const {
	return id_;
}
// #@@range_end(layer_id)

// #@@range_begin(layer_setget_window)
Layer& Layer::SetWindow(const std::shared_ptr<Window>& window) {
	window_ = window;
	return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const {
	return window_;
}
// #@@range_end(layer_setget_window)

// #@@range_begin(layer_move)
Layer& Layer::Move(Vector2D<int> pos) {
	pos_ = pos;
	return *this;
}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
	pos_ += pos_diff;
	return *this;
}
// #@@range_end(layer_move)

// #@@range_begin(layer_drawto)
void Layer::DrawTo(FrameBuffer& screen) const {
	if (window_) {
		window_->DrawTo(screen, pos_); // 레이어가 위치를 관리
	}
}
// #@@range_end(layer_drawto)


// #@@range_begin(layermgr_setwriter)
void LayerManager::SetWriter(FrameBuffer* screen) {
	screen_ = screen;
}
// #@@range_end(layermgr_setwriter)

// #@@range_begin(layermgr_newlayer)
// 생성한 레이어 인스턴스로 (Layer&) 반환 for NewLayer().SetWindow(...) 추가 설정
Layer& LayerManager::NewLayer() {
	++latest_id_;
	// emplace_back(): 지정한 값을 배열의 끝에 추가하는 메소드
	// .emplace_back() -> std::unique_ptr<Layer>& 타입 값 반환
	// 공유 불가 -> * 연산자를 통해 Layer& 타입으로 반환
	return *layers_.emplace_back(new Layer{latest_id_});
}
// #@@range_end(layermgr_newlayer)

// #@@range_begin(layermgr_draw)
void LayerManager::Draw() const {
	// layer_stack_ 배열은 선두가 가장 아래쪽, 배열 끝이 가장 위쪽
	for (auto layer : layer_stack_) {
		layer->DrawTo(*screen_);
	}
}
// #@@range_end(layermgr_draw)

// #@@range_begin(layermgr_move)
// FindLayer() ID 유효성 확인을 호출 측의 책임으로 위임 -> Move() Null 체크 생략

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
	FindLayer(id)->Move(new_position);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
	FindLayer(id)->MoveRelative(pos_diff);
}
// #@@range_end(layermgr_move)

// #@@range_begin(layermgr_updown)
void LayerManager::UpDown(unsigned int id, int new_height) {
	if (new_height < 0) { // 음수면 숨김
		Hide(id);
		return;
	}
	
	if (new_height > layer_stack_.size()) {
		new_height = layer_stack_.size();
	}
	
	auto layer = FindLayer(id);
	auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
	auto new_pos = layer_stack_.begin() + new_height;
	
	// 레이어 제거 필요 X, new_pos 위치에 레이어 삽입
	if (old_pos == layer_stack_.end()) { 
		layer_stack_.insert(new_pos, layer);
		return;
	}

	if (new_pos == layer_stack_.end()) {
		--new_pos; // 끝을 넘지 않게 하기 위함
	}
	layer_stack_.erase(old_pos);
	layer_stack_.insert(new_pos, layer);
}
// #@@range_end(layermgr_updown)

// #@@range_begin(layermgr_hide)
void LayerManager::Hide(unsigned int id) {
	auto layer = FindLayer(id);
	auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
	if (pos != layer_stack_.end()) {
		layer_stack_.erase(pos);
	}
}
// #@@range_end(layermgr_hide)

// #@@range_begin(layermgr_findlayer)
Layer* LayerManager::FindLayer(unsigned int id) {
	// 캡처: 람다식 외부에 있는 로컬 변수를 람다식 내부에서 사용하기 위함
	// 동일한 레이어 ID를 갖고 있는지 검사
	auto pred = [id](const std::unique_ptr<Layer>& elem) {
		return elem->ID() == id;
	};
	auto it = std::find_if(layers_.begin(), layers_.end(), pred);
	if (it == layers_.end()) {
		return nullptr;
	}
	// it: std::unique_ptr<Layer>, raw 포인터 Layer*를 얻으려면 "it->get()" 필요
	return it->get();
}
// #@@range_end(layermgr_findlayer)

LayerManager* layer_manager;