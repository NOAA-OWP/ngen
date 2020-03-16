#ifndef GEOJSON_FEATUREVISITOR_H
#define GEOJSON_FEATUREVISITOR_H

namespace geojson {
    class PointFeature;
    class LineStringFeature;
    class PolygonFeature;
    class MultiPointFeature;
    class MultiLineStringFeature;
    class MultiPolygonFeature;
    class CollectionFeature;

    class FeatureVisitor {
        public:
            virtual void visit(PointFeature *feature) = 0;
            virtual void visit(LineStringFeature *feature) = 0;
            virtual void visit(PolygonFeature *feature) = 0;
            virtual void visit(MultiPointFeature *feature) = 0;
            virtual void visit(MultiLineStringFeature *feature) = 0;
            virtual void visit(MultiPolygonFeature *feature) = 0;
            virtual void visit(CollectionFeature* feature) = 0;
    };
}
#endif