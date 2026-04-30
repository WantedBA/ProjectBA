# ExcelToJsonConverter - 빌드 안내

언리얼 에디터는 `ExcelToJsonConverter.exe` 를 직접 실행하므로, 팀원들은
별도로 파이썬을 설치할 필요가 없습니다. 스크립트(`ExcelToJsonConverter.py`)
를 수정했다면 개발자 머신에서 .exe 를 다시 빌드하고 결과물을 커밋하세요.

## 사전 준비 (개발자 머신 한정)

```
python -m pip install --upgrade pip
python -m pip install pandas openpyxl pyinstaller
```

## .exe 빌드

이 디렉터리(`BADesign/ExcelToJsonConverter`) 에서 실행:

```
pyinstaller --onefile --name ExcelToJsonConverter ExcelToJsonConverter.py
```

> `pyinstaller` 가 PATH 에 없으면 `python -m PyInstaller ...` 형태로 실행해도 됩니다.

PyInstaller 는 결과물을 `dist/ExcelToJsonConverter.exe` 에 출력합니다.
이 파일을 .py 와 같은 위치로 옮기고 중간 산출물은 정리합니다:

```
move dist\ExcelToJsonConverter.exe .
rmdir /s /q build dist
del ExcelToJsonConverter.spec
```

빌드된 `ExcelToJsonConverter.exe` 를 커밋하세요. 파일 크기가 커서
(pandas 가 번들되면 약 30~40MB) 필요하면 Git LFS 사용을 검토합니다.

> 참고: 저장소 루트의 `.gitignore` 에 `*.exe` 룰이 있으므로,
> 이 .exe 를 커밋하려면 화이트리스트 항목을 추가해야 합니다.
> 예: `!BADesign/ExcelToJsonConverter/ExcelToJsonConverter.exe`

## 에디터가 .exe 를 찾는 위치

`BAProjectEditor` 모듈의 `FBATableGenerator` 가 다음 경로를 호출합니다:

```
<Project>/BADesign/ExcelToJsonConverter/ExcelToJsonConverter.exe
```

작업 디렉터리는 위 폴더로 설정됩니다. .exe 는
`<Project>/BADesign/Excel/*.xlsx` 를 읽어
`<Project>/BADesign/Json/<파일명>.json` 으로 출력합니다.
