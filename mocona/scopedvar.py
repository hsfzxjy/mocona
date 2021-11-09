from mocona._scopedvar import S, V


class TemplateMeta(type):
    def __setattr__(cls, name, value):
        var = getattr(cls, name)
        S.assign(var, value)


class Template(metaclass=TemplateMeta):
    def __init_subclass__(cls, ns, *args, **kwargs):
        super().__init_subclass__(*args, **kwargs)
        annotations = getattr(cls, "__annotations__", None)
        if annotations is not None:
            for key in annotations:
                type.__setattr__(cls, key, S._varfor(getattr(ns, key)))
