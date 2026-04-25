from pydantic import BaseModel

class SDRConfig(BaseModel):
    antenna: str = "A"
    lna_gain: int = 0
