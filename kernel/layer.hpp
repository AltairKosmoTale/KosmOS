#pragma once
#include <memory>
#include <map>
#include <vector>
#include "graphics.hpp"
#include "window.hpp"

// #@@range_begin(layer)
class Layer {
 public:
	// 지정된 ID를 가진 layer를 생성
	Layer(unsigned int id = 0);
	// 이 인스턴스의 ID를 반환
	unsigned int ID() const;
	// 윈도우 설정 -> 기존 윈도우는 이 레이어에서 제외
	Layer& SetWindow(const std::shared_ptr<Window>& window);
	// 설정된 윈도우 반환
	std::shared_ptr<Window> GetWindow() const;
	Vector2D<int> GetPosition() const;
	Layer& SetDraggable(bool draggable);
	bool IsDraggable() const;
	// 레이어의 위치정보를 지정된 "절대 좌표"로 갱신, 다시 그리지는 않음
	Layer& Move(Vector2D<int> pos);
	// 레이어의 위치정보를 지정된 "상대 좌표"로 갱신, 다시 그리지는 않음
	Layer& MoveRelative(Vector2D<int> pos_diff);
	// wirter에 현재 설정된 윈도우의 내용을 렌더링
	void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

 private:
	unsigned int id_;
	Vector2D<int> pos_{};
	std::shared_ptr<Window> window_{}; // smart pointer
	bool draggable_{false};
};
// #@@range_end(layer)

// #@@range_begin(layer_manager)
class LayerManager {
 public:
	// Draw 메소드 등으로 렌더링할 때의 렌더링 목적지를 설정
	void SetWriter(FrameBuffer* screen);
	// 새로운 레이어를 생성 및 참조 반환
	// 새롭게 생성된 레이어의 실체는 LayerManager 내부의 컨테이너에서 유지
	Layer& NewLayer();
	// 현재 표시상태에 있는 레이어를 그림
	void Draw(const Rectangle<int>& area) const;
	void Draw(unsigned int id) const;
	// 레이어의 위치정보를 지정된 "절대 좌표"로 갱신, 다시 그림
	void Move(unsigned int id, Vector2D<int> new_pos);
	// 레이어의 위치정보를 지정된 "상대 좌표"로 갱신, 다시 그림
	void MoveRelative(unsigned int id, Vector2D<int> pos_diff);
	// 레이어의 높이 방향 위치를 지정된 위치로 이동
	// 음수 높이를 지정하면 표시 X, 0이상 지정시 높이 표시
	// 현재 레이어 수 이상의 수치 지정시, 최상단 레이어로 설정
	void UpDown(unsigned int id, int new_height);
	// 레이어를 숨김
	void Hide(unsigned int id);
	// 지정된 좌표에 창을 가지는 가장 위에 표시되어 있는 레이어를 찾음
	Layer* FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const;
	
 private:
	// #@@range_begin(layermgr_fields)
	FrameBuffer* screen_{nullptr};
	// mutable: const 메소드 내에서 변경 가능 // 남용이 좋지 않음
	// 메모리 할당 및 해제 -> slow
	// so, 최적화를 위해 멤버 변수로 정의
	// back_buffer_의 변경이 다른 메소드에 영향을 미치지 않음
	// LayerManager::Draw() const로 선언
	mutable FrameBuffer back_buffer_{}; 
	// layers_: 동적 배열 (표시 O, 표시 X 레이어 포함 존재하는 모든 레이어 저장)
	// shared_ptr: 스마트 포인터의 일종 // 공유 가능
	// unique_ptr: 스마트 포인터의 일종 // 공유 불가 -> 소유 명시 (LayerManager)
	std::vector<std::unique_ptr<Layer>> layers_{}; 
	std::vector<Layer*> layer_stack_{};
	unsigned int latest_id_{0};
	// #@@range_end(layermgr_fields)
	
	Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;
// #@@range_end(layer_manager)
