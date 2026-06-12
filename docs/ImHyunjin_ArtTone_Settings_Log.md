# Im Hyunjin Art Tone Settings Log

이 문서는 임현진 담당 작업 중 에디터에서 설정한 화면 분위기, 포스트 프로세스, 필터, 에셋, 테스트맵 변경 사항을 계속 누적 기록하는 작업 로그입니다.

## 기록 원칙

- 언리얼 에디터에서 만진 설정은 가능한 한 하나씩 기록한다.
- 맵 이름, 액터 이름, 설정 위치, 값, 의도를 같이 적는다.
- 분위기 작업은 먼저 테스트맵에서 확인하고, 바로 메인 레벨에 넣지 않는다.
- 어둡고 거친 분위기를 만들더라도 카드, 총알, 버튼, 생명 카드 같은 플레이 정보는 잘 보여야 한다.
- 픽셀화, CRT, VHS, 글리치 효과는 기본 색감과 조명 톤을 잡은 뒤 추가한다.

## 현재 프로젝트 연결 상태

- 실제 프로젝트 위치: `C:\Users\user\Documents\Unreal Projects\showdown`
- 예전 Rider 경로: `C:\Users\user\RiderProjects\showdown`
- 예전 Rider 경로는 새 프로젝트 위치를 가리키는 Junction으로 연결되어 있다.
- Epic Launcher 프로젝트 검색 경로:
  - `C:/Users/user/Documents/Unreal Projects`
- 언리얼 에디터 언어: 한글

## 담당 역할 기준

- 담당자: 임현진
- 주요 역할:
  - 연출 총괄
  - 사물형 UI 공동 작업
  - 화면 분위기 방향 설정
  - 레벨/맵 통합 관리
  - 카드 공개, 룰렛, 생명 카드, 베팅 연출의 시각적 방향 잡기

## 첫 작업 목표

### 목표

`ArtToneTest` 테스트맵을 만들고, 포스트 프로세스 볼륨으로 기본 화면 톤을 잡는다.

### 작업 순서

1. 현재 레벨을 테스트맵으로 저장한다.
   - 메뉴: `파일` -> `현재 레벨을 다른 이름으로 저장`
   - 저장 위치: `콘텐츠/Maps`
   - 맵 이름: `ArtToneTest`

2. 포스트 프로세스 볼륨을 추가한다.
   - 액터 검색어: `포스트 프로세스 볼륨`
   - 영어로만 검색될 경우: `Post Process Volume`

3. 화면 전체에 적용한다.
   - 월드 아웃라이너에서 `PostProcessVolume` 선택
   - 디테일 패널에서 `무한` 또는 `Infinite` 검색
   - `무한 범위 (언바운드)` / `Infinite Extent (Unbound)` 체크

4. 기본 분위기 값을 조절한다.

## 추천 초기 포스트 프로세스 값

아직 실제 적용 전 추천값입니다. 적용 후 실제 값으로 갱신합니다.

| 항목 | 한글 검색어 | 추천값 | 의도 |
| --- | --- | --- | --- |
| Exposure Compensation | 노출 | `-0.5` | 화면을 살짝 어둡게 만든다. |
| Saturation | 채도 | `0.7` | 색을 빼서 낡고 건조한 느낌을 만든다. |
| Contrast | 대비 | `1.15` | 어둠과 밝음의 차이를 조금 강하게 만든다. |
| Vignette Intensity | 비네트 | `0.4` | 화면 가장자리를 어둡게 해 폐쇄감을 만든다. |
| Film Grain Intensity | 그레인 | `0.25` | 화면에 거친 질감을 더한다. |

## 실제 적용 로그

### 기본 포스트 프로세스 1차

아래 값은 초반 분위기 테스트용 기준값입니다. 실제 에디터 값이 다르면 추후 수정합니다.

| 맵 | 액터 | 설정 | 값 | 의도 |
| --- | --- | --- | --- | --- |
| `ArtToneTest` | `PostProcessVolume` | `Infinite Extent (Unbound)` | On | 볼륨 위치와 상관없이 화면 전체에 적용 |
| `ArtToneTest` | `PostProcessVolume` | `Exposure Compensation` | `-0.5` | 화면을 살짝 어둡게 만들어 긴장감 형성 |
| `ArtToneTest` | `PostProcessVolume` | `Saturation` | `0.7` | 채도를 낮춰 낡고 건조한 톤 형성 |
| `ArtToneTest` | `PostProcessVolume` | `Contrast` | `1.15` | 어둠과 밝음 차이를 약간 강화 |
| `ArtToneTest` | `PostProcessVolume` | `Vignette Intensity` | `0.4` | 화면 가장자리 어둡게 처리 |
| `ArtToneTest` | `PostProcessVolume` | `Film Grain Intensity` | `0.25` | 거친 화면 질감 추가 |

앞으로 현진님이 에디터에서 값을 바꾸거나 에셋을 추가하면 이 아래에 날짜별로 누적 기록한다.

### 2026-06-12

- 프로젝트 폴더를 `C:\Users\user\Documents\Unreal Projects\showdown` 위치로 이동했다.
- `C:\Users\user\RiderProjects\showdown` 경로는 새 위치로 연결되는 Junction으로 설정했다.
- Epic Launcher가 `C:/Users/user/Documents/Unreal Projects` 경로를 프로젝트 검색 위치로 사용하고 있음을 확인했다.
- 현진님 작업 방향을 화면 필터, 포스트 프로세스, 픽셀화, VHS/CRT, 노이즈 등 기본 분위기 틀 잡기로 정리했다.
- 첫 에디터 작업은 `ArtToneTest` 테스트맵 생성 후 `Post Process Volume`을 추가하는 것으로 정했다.
- `ArtToneTest`에서 기본 포스트 프로세스 1차 값 적용을 진행한 것으로 기록했다.
- `콘텐츠/ArtTone` 폴더를 만들었다.
- `M_PP_Pixelate` 머티리얼을 만들었다.
- `M_PP_Pixelate`의 `Material Domain`을 `Post Process`로 설정했다.
- `M_PP_Pixelate` 머티리얼 그래프에 픽셀화 노드를 구성했다.
  - `ScreenPosition(ViewportUV)` -> `Mask(R,G)` -> `Multiply` -> `Floor` -> `Divide` -> `SceneTexture(PostProcessInput0).UVs`
  - `SceneTexture(PostProcessInput0).Color` -> `Emissive Color`
  - `Scalar Parameter`: `PixelCount`
- `M_PP_Pixelate`를 `PostProcessVolume`의 `Post Process Materials` / `Blendables`에 추가해 화면에 적용했다.
